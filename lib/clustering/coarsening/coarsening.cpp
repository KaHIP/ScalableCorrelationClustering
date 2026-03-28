/******************************************************************************
 * coarsening.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#include <clustering/coarsening/clustering/size_constraint_label_propagation.h>
#include <tools/tools.h>
#include "coarsening.h"
#include "contraction.h"
#include "data_structure/graph_hierarchy.h"
#include "definitions.h"
#include "stop_rules/stop_rules.h"
#include "quality_metrics.h"

coarsening::coarsening() {

}

coarsening::~coarsening() {

}

void coarsening::perform_coarsening(const PartitionConfig & partition_config, graph_access & G, graph_hierarchy & hierarchy) {
        // Variables
        PartitionConfig copy_of_partition_config = partition_config;
        size_constraint_label_propagation sclp;
        contraction contracter;
        simple_clustering_stop_rule coarsening_stop_rule(copy_of_partition_config, G.number_of_nodes());

        graph_access* finer = &G;
        NodeID no_of_coarser_vertices;
        NodeID labels_changed;

        // Label propagation coarsening
        graph_access* coarser          = NULL;
        CoarseMapping* coarse_mapping  = NULL;
        bool contraction_stop = false;
	do {
		coarser          = new graph_access();
		coarse_mapping	 = new CoarseMapping();
		sclp.match(copy_of_partition_config, *finer, *coarse_mapping, no_of_coarser_vertices, labels_changed);
		contracter.contract_clustering(copy_of_partition_config, *finer, *coarser, *coarse_mapping, no_of_coarser_vertices);
                hierarchy.push_back(finer, coarse_mapping);
                contraction_stop = coarsening_stop_rule.stop(finer->number_of_nodes(), no_of_coarser_vertices, labels_changed);

                finer = coarser;
	} while(contraction_stop);
        hierarchy.push_back(finer, NULL); // append the last created level

	// Initial Clustering
	std::vector<int> partition_map(finer->number_of_nodes());
	forall_nodes((*finer), node) {
		partition_map[node] = node;
	} endfor
	EdgeWeight edgecut1 = 0;
	EdgeWeight edgecut2 = 0;
        if(partition_config.graph_already_partitioned && !partition_config.force_new_initial_partitioning) {
		quality_metrics qm;
		edgecut1 = qm.edge_cut(*finer);
		edgecut2 = qm.edge_cut(*finer, &(partition_map[0]));
	} 
	if(edgecut2 < edgecut1 || !partition_config.graph_already_partitioned 
				|| partition_config.force_new_initial_partitioning) {
		forall_nodes((*finer), node) {
			finer->setPartitionIndex(node, partition_map[node]);
		} endfor
		finer->set_partition_count(finer->number_of_nodes());
	}

        // contracter and coarsening_stop_rule are stack-allocated
}
