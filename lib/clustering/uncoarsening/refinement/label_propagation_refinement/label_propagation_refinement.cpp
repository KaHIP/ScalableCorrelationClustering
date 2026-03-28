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

        std::queue< NodeID > Q_a, Q_b;
        std::queue< NodeID > * Q      = &Q_a;
        std::queue< NodeID > * next_Q = &Q_b;
        std::vector<char> QC_a(G.number_of_nodes(), 0), QC_b(G.number_of_nodes(), 0);
        std::vector<char> * Q_contained      = &QC_a;
        std::vector<char> * next_Q_contained = &QC_b;

        // Don't push all nodes into deque for iteration 0 - iterate permutation directly
        std::vector< PartitionID > max_blocks;
        for( int j = 0; j < partition_config.label_iterations_refinement; j++) {
                // First iteration: iterate permutation directly
                NodeID iter_size = (j == 0) ? G.number_of_nodes() : 0;
                for( NodeID qi = 0; qi < iter_size; qi++) {
                        NodeID node = permutation[qi];

                        PartitionID max_block = G.getPartitionIndex(node);
                        // Sweep 1: accumulate + cache partition IDs
                        { EdgeID e_begin = G.get_first_edge(node), e_end = G.get_first_invalid_edge(node);
                        NodeID deg = e_end - e_begin;
                        PartitionID blk_cache[32];
                        bool use_cache = (deg <= 32);

                        for(EdgeID e = e_begin; e < e_end; e++) {
                                PartitionID blk = G.getPartitionIndex(G.getEdgeTarget(e));
                                hash_map[blk] += G.getEdgeWeight(e);
                                if(use_cache) blk_cache[e - e_begin] = blk;
                        }

                        // Sweep 2: find max using cached blocks (no edge reads)
                        max_blocks.clear();
                        max_blocks.push_back(max_block);
                        EdgeWeight max_value = 0;

                        if(use_cache) {
                                for(NodeID i = 0; i < deg; i++) {
                                        PartitionID cur_block = blk_cache[i];
                                        EdgeWeight cur_value  = hash_map[cur_block];
                                        if(cur_value > max_value) {
                                                max_value = cur_value;
                                                max_block = cur_block;
                                                max_blocks.clear();
                                        }
                                        if(cur_value == max_value) {
                                                max_blocks.push_back(cur_block);
                                        }
                                }
                        } else {
                                for(EdgeID e = e_begin; e < e_end; e++) {
                                        PartitionID cur_block = G.getPartitionIndex(G.getEdgeTarget(e));
                                        EdgeWeight cur_value  = hash_map[cur_block];
                                        if(cur_value > max_value) {
                                                max_value = cur_value;
                                                max_block = cur_block;
                                                max_blocks.clear();
                                        }
                                        if(cur_value == max_value) {
                                                max_blocks.push_back(cur_block);
                                        }
                                }
                        }

                        // Sweep 3: reset using cached blocks
                        if(use_cache) {
                                for(NodeID i = 0; i < deg; i++) hash_map[blk_cache[i]] = 0;
                        } else {
                                for(EdgeID e = e_begin; e < e_end; e++) hash_map[G.getPartitionIndex(G.getEdgeTarget(e))] = 0;
                        }
                        }

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

                // Process queue for iterations > 0
                while( !Q->empty() ) {
                        NodeID node = Q->front();
                        Q->pop();
                        (*Q_contained)[node] = 0;

                        PartitionID max_block = G.getPartitionIndex(node);
                        { EdgeID qe_begin = G.get_first_edge(node), qe_end = G.get_first_invalid_edge(node);
                        NodeID qdeg = qe_end - qe_begin;
                        PartitionID qblk_cache[32];
                        bool quse_cache = (qdeg <= 32);

                        for(EdgeID e = qe_begin; e < qe_end; e++) {
                                PartitionID blk = G.getPartitionIndex(G.getEdgeTarget(e));
                                hash_map[blk] += G.getEdgeWeight(e);
                                if(quse_cache) qblk_cache[e - qe_begin] = blk;
                        }
                        max_blocks.clear();
                        max_blocks.push_back(max_block);
                        EdgeWeight max_value = 0;
                        if(quse_cache) {
                                for(NodeID i = 0; i < qdeg; i++) {
                                        PartitionID cur_block = qblk_cache[i];
                                        EdgeWeight cur_value  = hash_map[cur_block];
                                        if(cur_value > max_value) { max_value = cur_value; max_block = cur_block; max_blocks.clear(); }
                                        if(cur_value == max_value) { max_blocks.push_back(cur_block); }
                                }
                        } else {
                                for(EdgeID e = qe_begin; e < qe_end; e++) {
                                        PartitionID cur_block = G.getPartitionIndex(G.getEdgeTarget(e));
                                        EdgeWeight cur_value  = hash_map[cur_block];
                                        if(cur_value > max_value) { max_value = cur_value; max_block = cur_block; max_blocks.clear(); }
                                        if(cur_value == max_value) { max_blocks.push_back(cur_block); }
                                }
                        }

                        if(quse_cache) {
                                for(NodeID i = 0; i < qdeg; i++) hash_map[qblk_cache[i]] = 0;
                        } else {
                                for(EdgeID e = qe_begin; e < qe_end; e++) hash_map[G.getPartitionIndex(G.getEdgeTarget(e))] = 0;
                        }
                        }

                        int pos = random_functions::nextInt(0, max_blocks.size()-1);
                        max_block = max_blocks[pos];
                        bool changed_label = G.getPartitionIndex(node) != max_block;
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

        // Q, next_Q, Q_contained, next_Q_contained are stack-allocated

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
