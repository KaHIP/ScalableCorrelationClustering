//
// Created by Felix Hausberger on 08.10.21.
//

#include <argtable3.h>
#include <iomanip>
#include <lib/clustering/signed_graph_clusterer.h>
#include <tools/tools.h>
#include <mpi.h>
#include "tools/mpi_edge_weight.h"
#include "parse_parameters.h"
#include "partition/partition_config.h"
#include "data_structure/graph_access.h"
#include "timer.h"
#include "graph_io.h"
#include "random_functions.h"
#include "quality_metrics.h"

#define MIN(A,B) (((A)>(B))?(B):(A))
#define MAX(A,B) (((A)>(B))?(A):(B))

void write_log(std::string & filename, std::stringstream& filebuffer_string);

int main(int argn, char **argv) {
        MPI_Init(&argn, &argv);    /* starts MPI */
#ifdef EDGE_WEIGHT_DOUBLE
        std::cout << std::setprecision(17);
#endif
        std::stringstream filebuffer_string;

        // Reading the graph
        PartitionConfig partition_config;
        std::string graph_filename;
        EdgeWeight best_cut = std::numeric_limits<EdgeWeight>::max();

        bool is_graph_weighted = false;
        bool suppress_output   = false;
        bool recursive         = false;

        int ret_code = parse_parameters(argn, argv,
                        partition_config,
                        graph_filename,
                        is_graph_weighted,
                        suppress_output,
                        recursive);

        if(ret_code) {
                return 0;
        }

        partition_config.graph_filename = graph_filename.substr( graph_filename.find_last_of( '/' ) +1 );
        int rank, size;

        MPI_Comm communicator = MPI_COMM_WORLD;
        //MPI_Comm comm_shared = MPI_COMM_NULL;
        //MPI_Comm_split_type(communicator, 
        //MPI_COMM_TYPE_SHARED, 
        //0 [> key <], 
        //MPI_INFO_NULL, 
        //&comm_shared);
        //MPI_Info info_win = MPI_INFO_NULL;
        //MPI_Info_create(&info_win);
        //MPI_Info_set(info_win, "alloc_shared_noncontig", "true");
        //MPI_Win win_shared = MPI_WIN_NULL;
        //void * base_ptr = NULL;
        //MPI_Aint bytes = 1;


        MPI_Comm_rank( communicator, &rank);
        MPI_Comm_size( communicator, &size);

        //std::cout <<  "AB"  << std::endl;
        partition_config.LogDump(stdout);
        graph_access G;

        timer t;
        graph_io::readGraphWeighted(G, graph_filename);

        if(partition_config.input_partition != "") {
                std::cout <<  "reading input partition" << std::endl;
                graph_io::readPartition(G, partition_config.input_partition);
                partition_config.graph_already_partitioned = true;

                int k = 0;
                std::vector<bool> used_block(G.number_of_nodes(),false);
                std::vector<PartitionID> map_old_new(G.number_of_nodes(),0);
                forall_nodes(G, n) {
                        PartitionID old_block = G.getPartitionIndex(n);
                        if (!used_block[old_block]) {
                                used_block[old_block] = true;
                                map_old_new[old_block] = k++;
                        }
                        PartitionID new_block = map_old_new[old_block];
                        G.setPartitionIndex(n, new_block);
                } endfor
                partition_config.k = k;
                G.set_partition_count(partition_config.k);
        }

        if( rank == ROOT ) {
                std::cout << "io time: " << t.elapsed()  << std::endl;
                std::cout <<  "graph has " <<  G.number_of_nodes() <<  " nodes and " <<  G.number_of_edges() <<  " edges"  << std::endl;
                //MPI_Win_allocate_shared(bytes, sizeof(EdgeWeight) [> disp_unit <], info_win, comm_shared, &base_ptr, &win_shared);
                //partition_config.overall_best_cut = base_ptr;
                //*(partition_config.overall_best_cut) = std::numeric_limits<EdgeWeight>::max();
        } else {
                //int disp_unit;
                //MPI_Win_allocate_shared(0, sizeof(EdgeWeight) [> disp_unit <], info_win, comm_shared, &base_ptr, &win_shared);
                //MPI_Win_shared_query(win_shared, 0, &bytes, &disp_unit, &base_ptr);
                //partition_config.overall_best_cut = base_ptr;
        }

        srand(partition_config.seed+(rank*rank));
        random_functions::setSeed(partition_config.seed+(rank*rank));

        // ***************************** perform clustering ***************************************
        t.restart();
        quality_metrics qm;

        std::cout <<  "performing clustering!"  << std::endl;
        EdgeWeight local_best_cut = std::numeric_limits<EdgeWeight>::max();
        if(partition_config.time_limit == 0) {
                signed_graph_clusterer clusterer;
                clusterer.perform_signed_clustering(partition_config, G);
                best_cut = qm.edge_cut(G);
                if (best_cut < local_best_cut) local_best_cut = best_cut;
        } else {
                PartitionID* map = new PartitionID[G.number_of_nodes()];
                forall_nodes(G, node) {
                        map[node] = 0;
                } endfor
                PartitionID best_k = G.number_of_nodes();
                while(t.elapsed() < partition_config.time_limit) {
                        signed_graph_clusterer clusterer;
                        partition_config.graph_already_partitioned = false;
                        clusterer.perform_signed_clustering(partition_config, G);
                        EdgeWeight cut = qm.edge_cut(G);
                        if(cut < (local_best_cut)) {
                                std::cout << "Improved objective: " << cut << " Elapsed time: " << t.elapsed() << " Rank: " << rank << std::endl;
                                best_cut = cut;
                                local_best_cut = best_cut;
                                forall_nodes(G, node) {
                                        map[node] = G.getPartitionIndex(node);
                                } endfor
                                best_k = G.get_partition_count();
                        }
                        filebuffer_string <<  t.elapsed() <<  " " <<  cut <<  std::endl;
                }

                forall_nodes(G, node) {
                        G.setPartitionIndex(node, map[node]);
                } endfor
                delete[] map;
                partition_config.k = best_k;
                G.set_partition_count(partition_config.k);
        }
        MPI_Barrier(communicator);

        EdgeWeight overall_best_cut;
        MPI_Reduce(&local_best_cut, &overall_best_cut, 1, mpi_edge_weight_type(), MPI_MIN, 0, MPI_COMM_WORLD);

        int myrank = local_best_cut == overall_best_cut ? rank : 0;
        int minrank;

        MPI_Reduce(&myrank, &minrank, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

        if( overall_best_cut == best_cut && rank == minrank ) {
                std::cout <<  "time spent for partitioning " << t.elapsed()  << std::endl;

                // ******************************* done clustering *****************************************
                // output some information about the clustering that we have computed
                EdgeWeight cut =  qm.edge_cut(G);
                std::cout << "cut \t\t"         << cut				  << std::endl;
                //std::cout << "z_value\t\t"      << qm.z_value(G)		  << std::endl;
                //std::cout << "error_rate \t"    << qm.error_rate(G)               << std::endl;
                //std::cout << "cluster_count \t" << G.get_partition_count()        << std::endl;
                //std::cout << "finalobjective  " << cut				  << std::endl;
                //std::cout << "bnd \t\t"         << qm.boundary_nodes(G)           << std::endl;
                //std::cout << "modularity  \t"   << tools::calculate_modularity(G) << std::endl;
                //std::cout << "balance \t"       << qm.balance(G)                  << std::endl;
                //std::cout << "max_comm_vol \t"  << qm.max_communication_volume(G) << std::endl;

                if(partition_config.output_partition) {
                        // write the clustering to the disc
                        std::stringstream filename;
                        if(partition_config.filename_output.empty()) {
                                filename << "clustering";
                        } else {
                                filename << partition_config.filename_output;
                        }
                        graph_io::writePartition(G, filename.str());
                }
        }

        if( partition_config.mh_print_log ) {
                std::stringstream filename_stream;
                filename_stream << partition_config.filename_log <<
                        "_" <<  rank;

                std::string filename(filename_stream.str());
                write_log(filename, filebuffer_string);
        }

        //MPI_Info_free(&info_win);
        //MPI_Win_free(&win_shared);
        //MPI_Comm_free(&comm_shared);
        MPI_Finalize();
}

void write_log(std::string & filename, std::stringstream& filebuffer_string) {
        std::ofstream f(filename.c_str());
        f << filebuffer_string.str();
        f.close();
}

