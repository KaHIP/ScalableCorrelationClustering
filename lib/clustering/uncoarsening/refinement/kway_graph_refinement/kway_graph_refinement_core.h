/******************************************************************************
 * kway_graph_refinement_core.h 
 *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#ifndef KWAY_GRAPH_REFINEMENT_CORE_PVGY97EW
#define KWAY_GRAPH_REFINEMENT_CORE_PVGY97EW

#include <unordered_map>
#include <vector>

#include "data_structure/priority_queues/priority_queue_interface.h"
#include "definitions.h"
#include "kway_graph_refinement_commons.h"
#include "tools/random_functions.h"
#include "clustering/uncoarsening/refinement/quotient_graph_refinement/2way_fm_refinement/vertex_moved_hashtable.h"
#include "clustering/uncoarsening/refinement/refinement.h"

class kway_graph_refinement_core  {
        public:
                kway_graph_refinement_core( );
                virtual ~kway_graph_refinement_core();

                EdgeWeight single_kway_refinement_round(PartitionConfig & config, 
                                                        graph_access & G, 
                                                        boundary_starting_nodes & start_nodes, 
                                                        int step_limit, 
                                                        vertex_moved_hashtable & moved_idx );

                //EdgeWeight single_kway_refinement_round(PartitionConfig & config, 
                                                        //graph_access & G, 
                                                        //complete_boundary & boundary, 
                                                        //boundary_starting_nodes & start_nodes, 
                                                        //int step_limit, 
                                                        //vertex_moved_hashtable & moved_idx); 


         private:
               EdgeWeight single_kway_refinement_round_internal(PartitionConfig & config, 
                                                                graph_access & G, 
                                                                boundary_starting_nodes & start_nodes, 
                                                                int step_limit,
                                                                vertex_moved_hashtable & moved_idx,
                                                                bool compute_touched_partitions); 


                void init_queue_with_boundary(const PartitionConfig & config, 
                                              graph_access & G, 
                                              std::vector<NodeID> & bnd_nodes, 
                                              refinement_pq * queue, 
                                              vertex_moved_hashtable & moved_idx);

                inline bool move_node(PartitionConfig & config, 
                                      graph_access & G, 
                                      NodeID & node, 
                                      vertex_moved_hashtable & moved_idx, 
                                      refinement_pq * queue);

                inline void move_node_back(PartitionConfig & config, 
                                           graph_access & G, 
                                           NodeID & node,
                                           PartitionID & to, 
                                           vertex_moved_hashtable & moved_idx, 
                                           refinement_pq * queue);

                void initialize_partition_moves_array(PartitionConfig & config, 
                                                      std::vector<bool> & partition_move_valid); 
                
                kway_graph_refinement_commons* commons;
};

inline bool kway_graph_refinement_core::move_node(PartitionConfig & config, 
                graph_access & G, 
                NodeID & node, 
                vertex_moved_hashtable & moved_idx, 
                refinement_pq * queue) {


        //PartitionID from = G.getPartitionIndex(node);
        PartitionID to;
        EdgeWeight node_ext_deg;
        commons->compute_gain(G, node, to, node_ext_deg);
        //NodeWeight this_nodes_weight = G.getNodeWeight(node);

        G.setPartitionIndex(node, to);        

        //update gain of neighbors / the boundaries have already been updated
        const Edge* __restrict__ edges = G.edge_array();
        { EdgeID e_begin = G.get_first_edge(node), e_end = G.get_first_invalid_edge(node);
        for(EdgeID e = e_begin; e < e_end; e++) {
                NodeID target = edges[e].target;
                PartitionID targets_max_gainer;
                EdgeWeight ext_degree; // the local external degree
                Gain gain = commons->compute_gain(G, target, targets_max_gainer, ext_degree);

                if(queue->contains(target)) {
                        if(targets_max_gainer != INVALID_PARTITION) { // is boundary node? // before signed clustering: ext_degree > 0
                                queue->changeKey(target, gain);
                        } else {
                                queue->deleteNode(target);
                        }
                } else {
                        if(targets_max_gainer != INVALID_PARTITION) { // is boundary node? // before signed clustering: ext_degree > 0
                                if(moved_idx[target].index == NOT_QUEUED) {
                                        queue->insert(target, gain);
                                        moved_idx[target].index = NOT_MOVED;
                                        moved_idx.touch(target);
                                }
                        } 
                }
        }
        }

        return true;
}

#endif //[> end of include guard: KWAY_GRAPH_REFINEMENT_PVGY97EW <]

