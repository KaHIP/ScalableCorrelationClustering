/******************************************************************************
 * kway_graph_refinement.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#include <algorithm>
#include <unordered_map>

#include "kway_graph_refinement.h"
#include "kway_graph_refinement_core.h"
#include "kway_stop_rule.h"
#include "quality_metrics.h"
#include "random_functions.h"
#include "timer.h"

kway_graph_refinement::kway_graph_refinement() {
}

kway_graph_refinement::~kway_graph_refinement() {
}

EdgeWeight kway_graph_refinement::perform_refinement(PartitionConfig & config, graph_access & G) {

        kway_graph_refinement_core refinement_core;
        
        EdgeWeight overall_improvement = 0;
        int max_number_of_swaps        = (int)(G.number_of_nodes());
        bool sth_changed               = config.no_change_convergence;

        vertex_moved_hashtable moved_idx(G.number_of_nodes()); 
        for( unsigned i = 0; i < config.kway_rounds || sth_changed; i++) {
                EdgeWeight improvement = 0;    

                boundary_starting_nodes start_nodes;
                start_nodes.reserve(G.number_of_nodes());
                setup_start_nodes(config, G, start_nodes);

                if(start_nodes.size() == 0) return 0; // nothing to refine

                //std::cout <<  "kway search limit " <<  config.kway_fm_search_limit  << std::endl;
                //metis steplimit
                int step_limit = (int)((config.kway_fm_search_limit/100.0)*max_number_of_swaps);
                step_limit = std::max(step_limit, 15);
                step_limit = 15; // CS hard parameter default

                improvement += refinement_core.single_kway_refinement_round(config, G,  
                                                                            start_nodes, step_limit, 
                                                                            moved_idx);

                moved_idx.reset();
                sth_changed = improvement != 0 && config.no_change_convergence;
                if(improvement == 0) break; 
                overall_improvement += improvement; 

        } 

        ASSERT_TRUE(overall_improvement >= 0);

        /* PartitionID cur_no_clusters = 0; */
        /* std::unordered_map<PartitionID, PartitionID> remap; */
        /* forall_nodes(G, node) { */
        /*         PartitionID cur_cluster = G.getPartitionIndex(node); */
        /*         //check whether we already had that */
        /*         if( remap.find( cur_cluster ) == remap.end() ) { */
        /*             remap[cur_cluster] = cur_no_clusters++; */
        /*         } */

        /*         G.setPartitionIndex(node, remap[cur_cluster]); */
        /* } endfor */
        /* G.set_partition_count(cur_no_clusters); */
        /* config.k = cur_no_clusters; */

        return (EdgeWeight) overall_improvement; 
}

void kway_graph_refinement::setup_start_nodes(PartitionConfig & config, graph_access & G,  boundary_starting_nodes & start_nodes) {
        forall_nodes(G, node) {
                PartitionID block = G.getPartitionIndex(node);
                forall_out_edges(G, e, node) {
                        NodeID target = G.getEdgeTarget(e);
                        if( G.getPartitionIndex(target) != block) {
                                start_nodes.push_back(node);
                                break;
                        }
                } endfor
        } endfor
        
        //QuotientGraphEdges quotient_graph_edges;
        //boundary.getQuotientGraphEdges(quotient_graph_edges);

        ////std::unordered_map<NodeID, bool> allready_contained;
        //std::vector<bool> allready_contained(G.number_of_nodes(), false);

        //for( unsigned i = 0; i < quotient_graph_edges.size(); i++) {
                //boundary_pair & ret_value = quotient_graph_edges[i];
                //PartitionID lhs           = ret_value.lhs;
                //PartitionID rhs           = ret_value.rhs;

                //PartialBoundary & partial_boundary_lhs = boundary.getDirectedBoundary(lhs, lhs, rhs);
                //forall_boundary_nodes(partial_boundary_lhs, cur_bnd_node) {
                        //ASSERT_EQ(G.getPartitionIndex(cur_bnd_node), lhs);
                        //if(!allready_contained[cur_bnd_node]) {
                                ////allready_contained.find(cur_bnd_node) == allready_contained.end() ) { 
                                //start_nodes.push_back(cur_bnd_node);
                                //allready_contained[cur_bnd_node] = true;
                        //}
                //} endfor

                //PartialBoundary & partial_boundary_rhs = boundary.getDirectedBoundary(rhs, lhs, rhs);
                //forall_boundary_nodes(partial_boundary_rhs, cur_bnd_node) {
                        //ASSERT_EQ(G.getPartitionIndex(cur_bnd_node), rhs);
                        ////if(allready_contained.find(cur_bnd_node) == allready_contained.end()) { 
                        //if(!allready_contained[cur_bnd_node]) {
                                //start_nodes.push_back(cur_bnd_node);
                                //allready_contained[cur_bnd_node] = true;
                        //}
                //} endfor
        //}
}


