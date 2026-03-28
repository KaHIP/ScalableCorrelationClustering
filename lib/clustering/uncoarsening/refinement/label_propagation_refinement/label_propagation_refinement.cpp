/******************************************************************************
 * label_propagation_refinement.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#include "label_propagation_refinement.h"
#include "clustering/coarsening/clustering/node_ordering.h"
#include "definitions.h"
#include "tools/random_functions.h"

label_propagation_refinement::label_propagation_refinement() {
                
}

label_propagation_refinement::~label_propagation_refinement() {
                
}

EdgeWeight label_propagation_refinement::perform_refinement(PartitionConfig & partition_config, graph_access & G) {
	//random_functions::fastRandBool<uint64_t> random_obj;
        // coarse_mapping stores cluster id and the mapping (it is identical)
        //std::vector<NodeWeight> cluster_sizes(G.number_of_nodes(), 0);
        std::vector<EdgeWeight> hash_map(G.number_of_nodes(), 0);
        std::vector<NodeID> permutation(G.number_of_nodes());

        node_ordering n_ordering;
        n_ordering.order_nodes(partition_config, G, permutation);

        std::queue< NodeID > * Q             = new std::queue< NodeID >();
        std::queue< NodeID > * next_Q        = new std::queue< NodeID >();
        std::vector<bool> * Q_contained      = new std::vector<bool>(G.number_of_nodes(), false);
        std::vector<bool> * next_Q_contained = new std::vector<bool> (G.number_of_nodes(), false);

        forall_nodes(G, node) {
                //cluster_sizes[G.getPartitionIndex(node)] += G.getNodeWeight(node);
                Q->push(permutation[node]);
        } endfor

        //std::cout <<  "partition " <<  partition_config.label_iterations_refinement  << std::endl;
        std::vector< PartitionID > max_blocks;
        for( int j = 0; j < partition_config.label_iterations_refinement; j++) {
                while( !Q->empty() ) {
                        NodeID node = Q->front();
                        Q->pop();
                        (*Q_contained)[node] = false;

                        //now move the node to the cluster that is most common in the neighborhood
                        forall_out_edges(G, e, node) {
                                NodeID target = G.getEdgeTarget(e);
                                hash_map[G.getPartitionIndex(target)]+=G.getEdgeWeight(e);
                        } endfor

                        //second sweep for finding max and resetting array
                        PartitionID max_block = G.getPartitionIndex(node);
                        max_blocks.clear();
                        max_blocks.push_back(max_block);

                        /* EdgeWeight max_value = hash_map[max_block]; */
                        EdgeWeight max_value = 0;
                        forall_out_edges(G, e, node) {
                                NodeID target             = G.getEdgeTarget(e);
                                PartitionID cur_block     = G.getPartitionIndex(target);
                                EdgeWeight cur_value      = hash_map[cur_block];
                                if(cur_value > max_value)
                                {
                                        max_value = cur_value;
                                        max_block = cur_block;
                                        max_blocks.clear();
                                }

                                if(cur_value == max_value) {
                                        max_blocks.push_back(cur_block);
                                }
                        } endfor

                        forall_out_edges(G, e, node) {
                                NodeID target             = G.getEdgeTarget(e);
                                PartitionID cur_block     = G.getPartitionIndex(target);
                                hash_map[cur_block] = 0;
                        } endfor

                        int pos = random_functions::nextInt(0, max_blocks.size()-1);
                        max_block = max_blocks[pos];
                        //cluster_sizes[G.getPartitionIndex(node)]  -= G.getNodeWeight(node);
                        //cluster_sizes[max_block]                  += G.getNodeWeight(node);
                        bool changed_label                        = G.getPartitionIndex(node) != max_block;
                        G.setPartitionIndex(node, max_block);

                        if(changed_label) {
                                forall_out_edges(G, e, node) {
                                        NodeID target = G.getEdgeTarget(e);
                                        if(!(*next_Q_contained)[target]) {
                                                next_Q->push(target);
                                                (*next_Q_contained)[target] = true;
                                        } 
                                } endfor
                        }
                } 

                std::swap( Q, next_Q);
                std::swap( Q_contained, next_Q_contained);
        }

        /* remap_cluster_ids(partition_config, G); */

        delete Q;
        delete next_Q;
        delete Q_contained;
        delete next_Q_contained;

        return 0;
}

void label_propagation_refinement::remap_cluster_ids(PartitionConfig & partition_config, graph_access & G) {
    PartitionID cur_no_clusters = 0;
    std::vector<PartitionID> remap(G.number_of_nodes(), INVALID_PARTITION);
    forall_nodes(G, node) {
            PartitionID cur_cluster = G.getPartitionIndex(node);
            if( remap[cur_cluster] == INVALID_PARTITION ) {
                remap[cur_cluster] = cur_no_clusters++;
            }

            G.setPartitionIndex(node, remap[cur_cluster]);
    } endfor
    G.set_partition_count(cur_no_clusters);
    partition_config.k = cur_no_clusters;
}
