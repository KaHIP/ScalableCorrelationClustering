//
// Created by Felix Hausberger on 11.12.21.
//

#include <algorithm>
#include <iostream>
#include <mpi.h>
#include "tools/mpi_edge_weight.h"
#include "diversifyer.h"
#include "exchange/exchanger.h"
#include "graph_partitioner.h"
#include "evolutionary_signed_graph_clusterer.h"
#include "quality_metrics.h"
#include "random_functions.h"

evolutionary_signed_graph_clusterer::evolutionary_signed_graph_clusterer() : MASTER(0), m_time_limit(0) {
	m_best_global_objective = std::numeric_limits<EdgeWeight>::max();
	m_best_cycle_objective  = std::numeric_limits<EdgeWeight>::max();
	m_best_local_objective  = std::numeric_limits<EdgeWeight>::max();
	m_rounds                = 0;
	m_termination           = false;
	m_communicator          = MPI_COMM_WORLD;
	MPI_Comm_rank( m_communicator, &m_rank);
	MPI_Comm_size( m_communicator, &m_size);
}

evolutionary_signed_graph_clusterer::evolutionary_signed_graph_clusterer(MPI_Comm communicator) : MASTER(0), m_time_limit(0) {
	m_best_global_objective = std::numeric_limits<EdgeWeight>::max();
	m_best_cycle_objective  = std::numeric_limits<EdgeWeight>::max();
	m_best_local_objective  = std::numeric_limits<EdgeWeight>::max();
	m_rounds                = 0;
	m_termination           = false;
	m_communicator          = communicator;
	MPI_Comm_rank( m_communicator, &m_rank);
	MPI_Comm_size( m_communicator, &m_size);

}

evolutionary_signed_graph_clusterer::~evolutionary_signed_graph_clusterer() {
	delete[] m_best_global_map;
}

void evolutionary_signed_graph_clusterer::perform_evolutionary_signed_clustering(PartitionConfig & partition_config, graph_access & G) {
	m_time_limit      = partition_config.time_limit;
	m_island          = new population(m_communicator, partition_config);
	m_best_global_map = new PartitionID[G.number_of_nodes()];

	srand(partition_config.seed+(m_rank*m_rank));
	random_functions::setSeed(partition_config.seed+(m_rank*m_rank));

	PartitionConfig ini_working_config  = partition_config;
	//m_t.restart();
	initialize( ini_working_config, G);

	exchanger ex(m_communicator);
	do {
		PartitionConfig working_config  = partition_config;

		working_config.graph_already_partitioned  = false;
		working_config.no_new_initial_partitioning = false;

		working_config.mh_pool_size = ini_working_config.mh_pool_size;
		if(m_rounds == 0 && working_config.mh_enable_quickstart) {
			ex.quick_start( working_config, G, *m_island );
		}

		perform_local_clustering( working_config, G );
		/* if(m_rank == ROOT) { */
			/* std::cout <<  "t left " <<  (m_time_limit - m_t.elapsed()) << std::endl; */
		/* } */

		//push and recv
		if( m_t.elapsed() <= m_time_limit && m_size > 1) {
			unsigned messages = ceil(log(m_size));
			for( unsigned i = 0; i < messages; i++) {
				ex.push_best( working_config, G, *m_island );
				ex.recv_incoming( working_config, G, *m_island );
			}
		}

		m_rounds++;
	} while( m_t.elapsed() <= m_time_limit );

	collect_best_clustering(G, partition_config);
	/* m_island->print(); */

	//print logfile (for convergence plots)
	if( partition_config.mh_print_log ) {
		std::stringstream filename_stream;
		filename_stream << partition_config.filename_log <<
			"_" <<  m_rank;

		std::string filename(filename_stream.str());
		m_island->write_log(filename);
	}

	delete m_island;
}

void evolutionary_signed_graph_clusterer::initialize(PartitionConfig & working_config, graph_access & G) {
        quality_metrics qm;
	if(working_config.input_partition  != "") {
		Individuum ind = {NULL, 0, NULL};
		ind.objective     = qm.objective(working_config, G, working_config.input_assignments);
		ind.partition_map = working_config.input_assignments;
		ind.cut_edges     = new std::vector<EdgeID>();

		forall_nodes(G, node) {
			forall_out_edges(G, e, node) {
				NodeID target = G.getEdgeTarget(e);
				if(ind.partition_map[node] != ind.partition_map[target]) {
					ind.cut_edges->push_back(e);
				}
			} endfor
		} endfor
		m_island->insert(G, ind);
	}
	if(working_config.input_partition2 != "") {
		Individuum ind = {NULL, 0, NULL};
		ind.objective     = qm.objective(working_config, G, working_config.input_assignments2);
		ind.partition_map = working_config.input_assignments2;
		ind.cut_edges     = new std::vector<EdgeID>();

		forall_nodes(G, node) {
			forall_out_edges(G, e, node) {
				NodeID target = G.getEdgeTarget(e);
				if(ind.partition_map[node] != ind.partition_map[target]) {
					ind.cut_edges->push_back(e);
				}
			} endfor
		} endfor
		m_island->insert(G, ind);
	}

	// each PE performs a partitioning
	// estimate the runtime of a partitioner call
	// calculate the poolsize and async Bcast the poolsize.
	// recv. has to be sync
	Individuum first_one = {NULL, 0, NULL};
	//m_t.restart();
        working_config.global_cycle_iterations=2;
        m_t.restart();
	m_island->createIndividuum( working_config, G, first_one);


        std::cout << "Improved local objective: " << first_one.objective << " Elapsed time: " << m_t.elapsed() << " Rank: " << m_rank << std::endl;
        m_best_local_objective = first_one.objective;
	double time_spend = m_t.elapsed();
	m_island->insert(G, first_one);

	//compute S and Bcast
	int population_size = 1;
	double fraction     = working_config.mh_initial_population_fraction;
	int POPSIZE_TAG     = 10;

	if( m_rank == ROOT ) {
		double fraction_to_spend_for_IP = (double) m_time_limit / fraction;
		population_size                 = ceil(fraction_to_spend_for_IP / time_spend);

		for( int target = 1; target < m_size; target++) {
			MPI_Request rq;
			MPI_Isend(&population_size, 1, MPI_INT, target, POPSIZE_TAG, m_communicator, &rq);
		}
	} else {
		MPI_Status rst;
		MPI_Recv(&population_size, 1, MPI_INT, ROOT, POPSIZE_TAG, m_communicator, &rst);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	population_size = std::max(3, population_size);
	if(working_config.mh_easy_construction) {
		population_size = std::min(50, population_size);
	} else {
		population_size = std::min(100, population_size);
	}
	std::cout <<  "poolsize = " <<  population_size  << std::endl;

	//set S
	m_island->set_pool_size(population_size);
	working_config.mh_pool_size = population_size;

}

EdgeWeight evolutionary_signed_graph_clusterer::collect_best_clustering(graph_access & G, const PartitionConfig & config) {
	//perform clustering locally
	EdgeWeight min_objective = 0;
	m_island->apply_fittest(G, min_objective);

	EdgeWeight best_local_objective  = min_objective;
	EdgeWeight best_local_objective_m  = min_objective;
	EdgeWeight best_global_objective = 0;

	PartitionID* best_local_map = new PartitionID[G.number_of_nodes()];
	std::vector< NodeWeight > block_sizes(G.get_partition_count(),0);

	forall_nodes(G, node) {
		best_local_map[node] = G.getPartitionIndex(node);
		block_sizes[G.getPartitionIndex(node)]++;
	} endfor

	NodeWeight max_domain_weight = 0;
	for( unsigned i = 0; i < G.get_partition_count(); i++) {
		if( block_sizes[i] > max_domain_weight ) {
			max_domain_weight = block_sizes[i];
		}
	}

	MPI_Allreduce(&best_local_objective_m, &best_global_objective, 1, mpi_edge_weight_type(), MPI_MIN, m_communicator);

	if( best_global_objective == std::numeric_limits<EdgeWeight>::max()) {
		//no partition is feasible
		MPI_Allreduce(&best_local_objective, &best_global_objective, 1, mpi_edge_weight_type(), MPI_MIN, m_communicator);
	}

	int my_domain_weight   = best_local_objective == best_global_objective ?
		max_domain_weight : std::numeric_limits<int>::max();
	int best_domain_weight = (int)max_domain_weight;

	MPI_Allreduce(&my_domain_weight, &best_domain_weight, 1, MPI_INT, MPI_MIN, m_communicator);

	// now we know what the best objective is ... find the best balance
	int bcaster = best_local_objective == best_global_objective
		&& my_domain_weight == best_domain_weight ? m_rank : std::numeric_limits<int>::max();
	int g_bcaster = 0;

	MPI_Allreduce(&bcaster, &g_bcaster, 1, MPI_INT, MPI_MIN, m_communicator);
	MPI_Bcast(best_local_map, G.number_of_nodes(), MPI_INT, g_bcaster, m_communicator);

	forall_nodes(G, node) {
		G.setPartitionIndex(node, best_local_map[node]);
	} endfor

	delete[] best_local_map;

	return best_global_objective;
}

EdgeWeight evolutionary_signed_graph_clusterer::perform_local_clustering(PartitionConfig & working_config, graph_access & G) {

	quality_metrics qm;
	unsigned local_repetitions = working_config.local_partitioning_repetitions;

	if( working_config.mh_diversify ) {
		diversifyer div;
		div.diversify(working_config);
	}

	//start a new round
	for( unsigned i = 0; i < local_repetitions; i++) {
	//if( working_config.mh_diversify ) {
		diversifyer div;
		div.diversify(working_config);
	
		if( working_config.mh_no_mh ) {
			Individuum first_ind = {NULL, 0, NULL};

                        //working_config.global_cycle_iterations=5;
			m_island->createIndividuum(working_config, G, first_ind);
			m_island->insert(G, first_ind);
		} else {
			if( m_island->is_full() && !working_config.mh_disable_combine) {

				int decision = random_functions::nextInt(0,9);
				Individuum output = {NULL, 0, NULL};

				if(decision < working_config.mh_flip_coin) {
					m_island->mutate_random(working_config, G, output);
					m_island->insert(G, output);

                                        //Individuum first_rnd = {NULL, 0, NULL};
					//Individuum output2 = {NULL, 0, NULL};

                                        ////std::cout <<  "calling new mutate"  << std::endl;
                                        //m_island->get_one_individual_tournament(first_rnd);
                                        //m_island->mutate_random(working_config, G, first_rnd, output2);
                                        //m_island->insert(G, output2);
				} else {
					int combine_decision = random_functions::nextInt(0,9);
					Individuum first_rnd = {NULL, 0, NULL};
					Individuum second_rnd = {NULL, 0, NULL};
					if(working_config.mh_enable_tournament_selection) {
						m_island->get_two_individuals_tournament(first_rnd, second_rnd);
					} else {
						m_island->get_two_random_individuals(first_rnd, second_rnd);
					}

                                        if(combine_decision == 0) {
						m_island->combine(working_config, G, first_rnd, second_rnd, output);
                                        } else {
                                                m_island->combine_ensemble(working_config, G, first_rnd, second_rnd, output);
                                        }
					m_island->insert(G, output);
				}
			} else {
				Individuum first_ind = {NULL, 0, NULL};
				if(m_island->is_full()) {
					m_island->mutate_random(working_config, G, first_ind);
				} else {
					m_island->createIndividuum(working_config, G, first_ind);
				}
				m_island->insert(G, first_ind);
			}
		}

                Individuum best_ind = {NULL, 0, NULL};
                m_island->get_best_individuum(best_ind);

                if( best_ind.objective < m_best_local_objective) {
                        m_best_local_objective = best_ind.objective;
                        std::cout << "Improved local objective: " << m_best_local_objective << " Elapsed time: " << m_t.elapsed() << " Rank: " << m_rank << std::endl;
                }

		//try to combine to random individuals from pool
		if( m_t.elapsed() > m_time_limit ) {
			break;
		}

	}

	EdgeWeight min_objective = 0;
	m_island->apply_fittest(G, min_objective);

	return min_objective;
}
