//
// Created by Felix Hausberger on 11.12.21.
//


#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <math.h>
#include <mpi.h>

#include "clustering/signed_graph_clusterer.h"
#include "population.h"
#include "quality_metrics.h"
#include "random_functions.h"
#include "timer.h"
#include "clustering/coarsening/clustering/size_constraint_label_propagation.h"
#include "graph_extractor.h"
#include "configuration.h"

population::population( MPI_Comm communicator, const PartitionConfig & partition_config ) {
        m_population_size    = partition_config.mh_pool_size;
        m_time_stamp         = 0;
        m_communicator       = communicator;
        m_global_timer.restart();
}

population::~population() {
        for( unsigned i = 0; i < m_internal_population.size(); i++) {
                delete[] (m_internal_population[i].partition_map);
                delete m_internal_population[i].cut_edges;
        }
}

void population::set_pool_size(int size) {
        m_population_size = size;
}

void population::createIndividuum(const PartitionConfig & config, graph_access & G, Individuum & ind) {

        PartitionConfig copy = config;
        signed_graph_clusterer clusterer;
        quality_metrics qm;

        std::ofstream ofs;
        std::streambuf* backup = std::cout.rdbuf();
        ofs.open("/dev/null");
        std::cout.rdbuf(ofs.rdbuf()); 

        timer t; t.restart();

        // diversify parameters
        
        clusterer.perform_signed_clustering(copy, G);
	copy.k = G.number_of_nodes();
	G.set_partition_count(copy.k);
        ofs.close();
        std::cout.rdbuf(backup);

        int* partition_map = new int[G.number_of_nodes()];

        forall_nodes(G, node) {
                partition_map[node] = G.getPartitionIndex(node);
        } endfor

        ind.objective     = qm.objective(config, G, partition_map);
        ind.partition_map = partition_map;
        ind.cut_edges     = new std::vector<EdgeID>();

        forall_nodes(G, node) {
                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if(partition_map[node] != partition_map[target]) {
                                ind.cut_edges->push_back(e);
                        }
                } endfor
        } endfor

        m_filebuffer_string <<  m_global_timer.elapsed() <<  " " <<  ind.objective <<  std::endl;
        m_time_stamp++;
}

void population::insert(graph_access & G, Individuum & ind) {
        if(m_internal_population.size() < m_population_size) {
                m_internal_population.push_back(ind);
        } else {
                EdgeWeight worst_objective = std::numeric_limits<EdgeWeight>::lowest();
                for( unsigned i = 0; i < m_internal_population.size(); i++) {
                        if(m_internal_population[i].objective > worst_objective) {
                                worst_objective = m_internal_population[i].objective;
                        }
                }         
                if(ind.objective > worst_objective ) {
                        delete[] (ind.partition_map);
                        delete ind.cut_edges;
                        return; // do nothing
                }
                //else measure similarity
                unsigned max_similarity = std::numeric_limits<unsigned>::max();
                unsigned max_similarity_idx = 0;
                for( unsigned i = 0; i < m_internal_population.size(); i++) {
                        // Get individual with the must similar cut edges
                        if(m_internal_population[i].objective >= ind.objective) {
                                //now measure
				int diff_size = m_internal_population[i].cut_edges->size() + ind.cut_edges->size();
				std::vector<EdgeID> output_diff(diff_size,std::numeric_limits<EdgeID>::max());

                                set_symmetric_difference(m_internal_population[i].cut_edges->begin(),
                                                         m_internal_population[i].cut_edges->end(),
                                                         ind.cut_edges->begin(),
                                                         ind.cut_edges->end(),
                                                         output_diff.begin());

                                unsigned similarity = 0;
                                for( unsigned j = 0; j < output_diff.size(); j++) {
                                        if(output_diff[j] < std::numeric_limits<EdgeID>::max()) {
                                                similarity++;
                                        } else {
                                                break;
                                        }
                                }

                                if( similarity < max_similarity) {
                                        max_similarity     = similarity;
                                        max_similarity_idx = i;
                                }
                        }
                }         

                delete[] (m_internal_population[max_similarity_idx].partition_map);
                delete m_internal_population[max_similarity_idx].cut_edges;

                m_internal_population[max_similarity_idx] = ind;
        }
}

void population::replace(Individuum & in, Individuum & out) {
        //first find it:
        for( unsigned i = 0; i < m_internal_population.size(); i++) {
                if(m_internal_population[i].partition_map == in.partition_map) {
                        //found it
                        delete[] (m_internal_population[i].partition_map);
                        delete m_internal_population[i].cut_edges;

                        m_internal_population[i] = out;
                        break;
                }
        }
}

void population::combine(const PartitionConfig & partition_config, 
                         graph_access & G, 
                         Individuum & first_ind, 
                         Individuum & second_ind, 
                         Individuum & output_ind) {

        PartitionConfig config = partition_config;
        G.resizeSecondPartitionIndex(G.number_of_nodes());
        if( first_ind.objective < second_ind.objective ) {
                forall_nodes(G, node) {
                        G.setPartitionIndex(node, first_ind.partition_map[node]);
                        G.setSecondPartitionIndex(node, second_ind.partition_map[node]);
                } endfor
        } else {
                forall_nodes(G, node) {
                        G.setPartitionIndex(node, second_ind.partition_map[node]);
                        G.setSecondPartitionIndex(node, first_ind.partition_map[node]);
                } endfor
        }

        config.combine                     = true;
        config.graph_already_partitioned   = true;
        config.no_new_initial_partitioning = false;
	config.force_new_initial_partitioning = false;

        createIndividuum(config, G, output_ind);
        /* std::cout <<  "objective " <<  output_ind.objective << std::endl; */
}

void population::combine_ensemble(const PartitionConfig & partition_config,
                                  graph_access & G,
                                  Individuum & first_ind,
                                  Individuum & second_ind,
                                  Individuum & output_ind) {

        PartitionConfig config = partition_config;
        G.resizeSecondPartitionIndex(G.number_of_nodes());

        hash_ensemble new_mapping;
        PartitionID no_of_coarse_vertices = 0;

        forall_nodes(G, node) {
                ensemble_pair cur_pair;
                cur_pair.lhs = first_ind.partition_map[node];
                cur_pair.rhs = second_ind.partition_map[node];
                cur_pair.n   = G.number_of_nodes();

                if(new_mapping.find(cur_pair) == new_mapping.end()) {
                        new_mapping[cur_pair].mapping = no_of_coarse_vertices;
                        no_of_coarse_vertices++;
                }

                G.setPartitionIndex(node, new_mapping[cur_pair].mapping);
                G.setSecondPartitionIndex(node, new_mapping[cur_pair].mapping);
        } endfor

        config.combine                     = true;
        config.graph_already_partitioned   = true;
        config.no_new_initial_partitioning = false;
	config.force_new_initial_partitioning = true;

        createIndividuum(config, G, output_ind);
}

void population::mutate_random( const PartitionConfig & partition_config, graph_access & G, Individuum & first_ind, Individuum & output) {
        PartitionConfig wconfig = partition_config;
        Individuum for_comb;
        wconfig.combine					= false;
        wconfig.graph_already_partitioned		= false;

        createIndividuum( wconfig, G, for_comb);

        combine_ensemble(partition_config, G, first_ind, for_comb, output);
}
//void population::combine_cross(const PartitionConfig & partition_config, 
                               //graph_access & G,
                               //Individuum & first_ind,
                               //Individuum & output_ind) {

        //PartitionConfig config = partition_config;
        //G.resizeSecondPartitionIndex(G.number_of_nodes());

        //int lowerbound = config.k / 4;
        //lowerbound     = std::max(2, lowerbound);
        //int kfactor    = random_functions::nextInt(lowerbound,4*config.k);
        //kfactor = std::min( kfactor, (int)G.number_of_nodes());

        //if( config.mh_cross_combine_original_k ) {
                //MPI_Bcast(&kfactor, 1, MPI_INT, 0, m_communicator);
        //}

        //unsigned larger_imbalance = random_functions::nextInt(config.epsilon,25);
        //// double epsilon = larger_imbalance/100.0;

        
        //PartitionConfig cross_config                      = config;
        //// cross_config.k                                    = kfactor;
        //cross_config.kaffpa_perfectly_balanced_refinement = false;
        //// cross_config.upper_bound_partition                = (1+epsilon)*ceil(partition_config.largest_graph_weight/(double)partition_config.k);
        //cross_config.refinement_scheduling_algorithm      = REFINEMENT_SCHEDULING_ACTIVE_BLOCKS;
        //cross_config.combine                              = false;
        //cross_config.graph_already_partitioned            = false;

	    //std::ofstream ofs;
	    //std::streambuf* backup = std::cout.rdbuf();
        //ofs.open("/dev/null");
        //std::cout.rdbuf(ofs.rdbuf()); 

        //signed_graph_clusterer clusterer;
        //clusterer.perform_signed_clustering(cross_config, G);
	//cross_config.k = G.number_of_nodes();
	//G.set_partition_count(cross_config.k);

        //ofs.close();
        //std::cout.rdbuf(backup);

        //forall_nodes(G, node) {
                //G.setSecondPartitionIndex(node, G.getPartitionIndex(node));
                //G.setPartitionIndex(node, first_ind.partition_map[node]);
        //} endfor

        //config.combine                    = true;
        //config.graph_already_partitioned  = true;

        //createIndividuum(config, G, output_ind);
        //std::cout << "objective cross combine " << output_ind.objective
                  //<< " k "                      << kfactor
                  //<< " imbalance "              << larger_imbalance
                  //<< " improvement "            << (first_ind.objective - output_ind.objective) << std::endl;
//}

void population::mutate_random( const PartitionConfig & partition_config, graph_access & G, Individuum & first_ind) {
        int number = random_functions::nextInt(0,5);

        PartitionConfig config				= partition_config;
        config.combine					= false;
        config.graph_already_partitioned		= true;
        config.block_cut_edges_only_in_first_level	= true;
        get_random_individuum(first_ind);

        if(number < 5) {
                forall_nodes(G, node) {
                        G.setPartitionIndex(node, first_ind.partition_map[node]);
                } endfor
                createIndividuum( config, G, first_ind);

        } else {
                forall_nodes(G, node) {
                        G.setPartitionIndex(node, first_ind.partition_map[node]);
                } endfor
                config.graph_already_partitioned  = false;
                createIndividuum( config, G, first_ind);
        }
}

void population::get_two_random_individuals(Individuum & first, Individuum & second) {
        int first_idx = random_functions::nextInt(0, m_internal_population.size()-1);
        first = m_internal_population[first_idx];

        int second_idx = random_functions::nextInt(0, m_internal_population.size()-1);
        while( first_idx == second_idx ) {
                second_idx = random_functions::nextInt(0, m_internal_population.size()-1);
        }

        second = m_internal_population[second_idx];
}

void population::get_one_individual_tournament(Individuum & first) {
        Individuum one = {NULL, 0, NULL};
        Individuum two = {NULL, 0, NULL};
        get_two_random_individuals(one, two);
        first  =  one.objective < two.objective ? one : two;
}

void population::get_two_individuals_tournament(Individuum & first, Individuum & second) {
        Individuum one, two;
        get_two_random_individuals(one, two);
        first  =  one.objective < two.objective? one : two;

        get_two_random_individuals(one, two);
        second =  one.objective < two.objective ? one : two;

        if( first.objective == second.objective) {
                second = one.objective >= two.objective? one : two;
        }
}

void population::get_random_individuum(Individuum & ind) {
        int idx = random_functions::nextInt(0, m_internal_population.size()-1);
        ind     = m_internal_population[idx];
}

void population::get_best_individuum(Individuum & ind) {
        EdgeWeight min_objective = std::numeric_limits<EdgeWeight>::max();
        unsigned idx = 0;

        for( unsigned i = 0; i < m_internal_population.size(); i++) {
                if((EdgeWeight) m_internal_population[i].objective < min_objective) {
                        min_objective = m_internal_population[i].objective;
                        idx           = i;
                }
        }

        ind = m_internal_population[idx];
}

bool population::is_full() {
        return m_internal_population.size() == m_population_size;
}

void population::apply_fittest( graph_access & G, EdgeWeight & objective ) {
        EdgeWeight min_objective = std::numeric_limits<EdgeWeight>::max();
	double best_balance      = std::numeric_limits<EdgeWeight>::max();
        unsigned idx             = 0;

	    quality_metrics qm;
        for( unsigned i = 0; i < m_internal_population.size(); i++) {
		        forall_nodes(G, node) {
			            G.setPartitionIndex(node, m_internal_population[i].partition_map[node]);
		        } endfor
		        double cur_balance = qm.balance(G);

                if((EdgeWeight) m_internal_population[i].objective < min_objective ||
                   ((EdgeWeight) m_internal_population[i].objective == min_objective && cur_balance < best_balance)) {
                        min_objective = m_internal_population[i].objective;
                        idx           = i;
			            best_balance  = cur_balance;
                }
        }

        forall_nodes(G, node) {
                G.setPartitionIndex(node, m_internal_population[idx].partition_map[node]);
        } endfor

        objective = min_objective;
}

void population::print() {
        int rank;
        MPI_Comm_rank( m_communicator, &rank);
        
        std::cout <<  "rank " <<  rank << " fingerprint ";

        for( unsigned i = 0; i < m_internal_population.size(); i++) {
                std::cout <<  m_internal_population[i].objective << " ";
        }         

        std::cout <<  std::endl;
}

void population::write_log(std::string & filename) {
        std::ofstream f(filename.c_str());
        f << m_filebuffer_string.str();
        f.close();
}


void population::mutate( const PartitionConfig & partition_config, graph_access & G, Individuum & first_ind, Individuum & second_ind, Individuum & output_ind) {
        Individuum output_a;
        Individuum output_b;
        mutate_random_incclusters(partition_config, G, first_ind, output_a);
        mutate_random_incclusters(partition_config, G, second_ind, output_b);
        //combine_improved_multilevel(partition_config, G, output_a, output_b, output_ind);

        delete[] (output_a.partition_map);
        delete output_a.cut_edges;

        delete[] (output_b.partition_map);
        delete output_b.cut_edges;
}

void population::mutate_random_incclusters( const PartitionConfig & partition_config, graph_access & G, Individuum & first_ind, Individuum & output_ind) {

        std::cout <<  "indput has " <<  first_ind.objective  << std::endl;
        std::vector< unsigned > clustering(G.number_of_nodes(), 0);
        std::mt19937 gen{ std::random_device{}() };
        forall_nodes(G, node) {
                clustering[node] = first_ind.partition_map[node];
                G.setPartitionIndex(node, clustering[node]);
        } endfor

        //relabel and count
        int k = 0;
        std::vector<bool> used_block(G.number_of_nodes(),false);
        std::vector<PartitionID> map_old_new(G.number_of_nodes(),0);
        forall_nodes(G, node) {
                PartitionID old_block = G.getPartitionIndex(node);
                if (!used_block[old_block]) {
                        used_block[old_block] = true;
                        map_old_new[old_block] = k++;
                }
                PartitionID new_block = map_old_new[old_block];
                G.setPartitionIndex(node, new_block);
                clustering[node] = new_block;
        } endfor
        std::cout <<  "k " << k      << std::endl;
        G.set_partition_count(G.number_of_nodes());
        

        double l = random_functions::nextDouble(0.01,partition_config.mh_mutate_fraction);
        unsigned c = *std::max_element(clustering.begin(), clustering.end()) + 1;
        unsigned clusters_to_select = std::ceil(l * c);

        std::cout << "mutating with l = " << l << " (clusters_to_select = " << clusters_to_select << ")\n";
        std::cout <<  "c " <<  c  << std::endl;

        //assert(c != 0);
        std::uniform_int_distribution<size_t> dist{ 0, c - 1 };
        std::set<unsigned> selected_clusters;
        for(size_t i = 0; i < clusters_to_select; ++i) {
                while(!selected_clusters.insert(dist(gen)).second);
        }
        ////G.set_partition_count(G.get_partition_count_compute());

        //std::cout <<  "A "  << std::endl;
        std::uniform_real_distribution<double> dist_eps{ 0.1, 0.5 };
        for(unsigned cluster: selected_clusters) {
                graph_access E;
                std::vector<unsigned> mapping;
                graph_extractor{ }.extract_block(G, E, cluster, mapping);

                PartitionConfig working_config;

                configuration cfg;
                //partition_config.k         = 2;
                cfg.clustering(working_config);

                signed_graph_clusterer{}.perform_signed_clustering(working_config, E);

                //std::cout <<  "B"  << std::endl;
                std::set<unsigned> new_clusters;
                for(size_t i = 0; i < mapping.size(); ++i) {
                        clustering[mapping[i]] = k + E.getPartitionIndex(i) + 1;
                        G.setPartitionIndex(mapping[i], k + E.getPartitionIndex(i) + 1);
                        new_clusters.insert(E.getPartitionIndex(i));
                        //if(E.getPartitionIndex(i) == ) { clustering[mapping[i]] = c; }
                }
                //std::cout <<  "C"  << std::endl;
                k += new_clusters.size();
                //std::cout <<  "new clusters " <<  new_clusters.size()  << std::endl;
                ++c;
        }

        {       int k = 0;
        std::vector<bool> used_block(G.number_of_nodes(),false);
        std::vector<PartitionID> map_old_new(G.number_of_nodes(),0);
        forall_nodes(G, node) {
                PartitionID old_block = G.getPartitionIndex(node);
                if (!used_block[old_block]) {
                        used_block[old_block] = true;
                        map_old_new[old_block] = k++;
                }
                PartitionID new_block = map_old_new[old_block];
                G.setPartitionIndex(node, new_block);
                clustering[node] = new_block;
        } endfor
        std::cout <<  "k_new " << k      << std::endl;}
        
        int* partition_map = new int[G.number_of_nodes()];
        forall_nodes(G, node) {
                partition_map[node] = clustering[node];
        } endfor

        //G.set_partition_count(G.get_partition_count_compute());
        quality_metrics qm;
        output_ind.objective = qm.objective(partition_config, G, partition_map);
        output_ind.partition_map    = partition_map;
        output_ind.cut_edges        = new std::vector<EdgeID>();
        std::cout <<  "output has " <<  output_ind.objective  << std::endl;

        forall_nodes(G, node) {
                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if(partition_map[node] != partition_map[target]) {
                                output_ind.cut_edges->push_back(e);
                        }
                } endfor
        } endfor

        //std::cout <<  "leaving"  << std::endl;

}


