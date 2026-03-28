/******************************************************************************
 * contraction.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#include "contraction.h"
#include "macros_assertions.h"

contraction::contraction() {

}

contraction::~contraction() {

}

void contraction::contract_clustering(const PartitionConfig & partition_config,
                              graph_access & G,
                              graph_access & coarser,
                              const CoarseMapping & coarse_mapping,
                              const NodeID & no_of_coarse_vertices) const {

        if(partition_config.combine) {
                coarser.resizeSecondPartitionIndex(no_of_coarse_vertices);
        }

        // Build quotient graph directly using coarse_mapping.
        // Avoids: complete_boundary overhead (16MB m_block_infos allocation),
        //         partition_map save/restore (8MB), vector-of-vectors block_list_nodes.

        // Group fine nodes by coarse block via counting sort: O(N)
        std::vector<NodeID> block_start(no_of_coarse_vertices + 1, 0);
        forall_nodes(G, node) {
                block_start[coarse_mapping[node] + 1]++;
        } endfor
        for(NodeID b = 1; b <= no_of_coarse_vertices; b++) {
                block_start[b] += block_start[b-1];
        }
        std::vector<NodeID> ordered_nodes(G.number_of_nodes());
        {
                std::vector<NodeID> block_pos(block_start.begin(), block_start.end());
                forall_nodes(G, node) {
                        ordered_nodes[block_pos[coarse_mapping[node]]++] = node;
                } endfor
        }

        // Build coarse graph: iterate blocks in order, accumulate cross-block edges
        std::vector<EdgeWeight> target_edgeweight(no_of_coarse_vertices, 0);
        std::vector<bool> target_added(no_of_coarse_vertices, false);
        std::vector<PartitionID> list_targets;

        coarser.start_construction(no_of_coarse_vertices, G.number_of_edges());

        for(NodeID p = 0; p < no_of_coarse_vertices; p++) {
                NodeID cn = coarser.new_node();
                coarser.setNodeWeight(cn, 1);

                for(NodeID idx = block_start[p]; idx < block_start[p+1]; idx++) {
                        NodeID node = ordered_nodes[idx];
                        forall_out_edges(G, e, node) {
                                NodeID target = G.getEdgeTarget(e);
                                NodeID target_block = coarse_mapping[target];
                                if(p != target_block) {
                                        if(!target_added[target_block]) {
                                                list_targets.push_back(target_block);
                                                target_added[target_block] = true;
                                        }
                                        target_edgeweight[target_block] += G.getEdgeWeight(e);
                                }
                        } endfor
                }

                for(unsigned ti = 0; ti < list_targets.size(); ti++) {
                        PartitionID tp = list_targets[ti];
                        EdgeID e = coarser.new_edge(p, tp);
                        coarser.setEdgeWeight(e, target_edgeweight[tp]);
                        target_edgeweight[tp] = 0;
                        target_added[tp] = false;
                }
                list_targets.clear();
        }

        coarser.finish_construction();

        // Project partition indices to coarse graph
        forall_nodes(G, node) {
                coarser.setPartitionIndex(coarse_mapping[node], G.getPartitionIndex(node));
                if(partition_config.combine) {
                        coarser.setSecondPartitionIndex(coarse_mapping[node], G.getSecondPartitionIndex(node));
                }
        } endfor

}
