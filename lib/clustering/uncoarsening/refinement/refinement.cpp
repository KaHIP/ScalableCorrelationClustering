/******************************************************************************
 * refinement.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

//#include <clustering/uncoarsening/refinement/quotient_graph_refinement/quotient_graph_refinement.h>
#include <clustering/uncoarsening/refinement/label_propagation_refinement/label_propagation_refinement.h>
#include <clustering/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement.h>
#include <clustering/uncoarsening/refinement/kway_graph_refinement/multitry_kway_fm.h>
#include "refinement.h"
#include "timer.h"
#include "tools/quality_metrics.h"

refinement::refinement() {
                
}

refinement::~refinement() {
                
}

EdgeWeight refinement::perform_refinement(PartitionConfig & config, graph_access * G) {
    //quality_metrics qm;
    EdgeWeight overall_improvement = 0;
    //complete_boundary* boundary;
    label_propagation_refinement label_propagation_obj;
    kway_graph_refinement kway_obj;
    label_propagation_refinement* label_propagation = &label_propagation_obj;
    kway_graph_refinement* kway                     = &kway_obj;
    //multitry_kway_fm* multitry_kway                 = new multitry_kway_fm();
    config.k = G->number_of_nodes();
    G->set_partition_count(config.k);

    /* std::cout << "\nedge-cut   : " << qm.edge_cut(*G) << "\n"; */

    timer t;
    t.restart();
    // Label Propagation
    if (!config.disable_label_propagation) {
	    //int k = 0;
	    //graph_access & G_ref = *G;
	    //std::vector<bool> used_block(G_ref.number_of_nodes(),false);
	    //std::vector<PartitionID> map_old_new(G_ref.number_of_nodes(),0);
	    //forall_nodes(G_ref, n) {
		    //PartitionID old_block = G_ref.getPartitionIndex(n);
		    //if (!used_block[old_block]) {
			    //used_block[old_block] = true;
			    //map_old_new[old_block] = k++;
		    //}
		    //PartitionID new_block = map_old_new[old_block];
		    //G_ref.setPartitionIndex(n, new_block);
	    //} endfor
	    //config.k = k;
	    //G_ref.set_partition_count(config.k);

	    overall_improvement += label_propagation->perform_refinement(config, *G);
	    /* std::cout << "edge-cut LP: " << qm.edge_cut(*G) << "\n"; */
    }
    //std::cout <<  "LP refinement " <<  t.elapsed()  << std::endl;
    t.restart();

    // Quotient Graph FM Local Search
    //if (!config.disable_quotient_refinement) {
	    //int k = 0;
	    //graph_access & G_ref = *G;
	    //std::vector<bool> used_block(G_ref.number_of_nodes(),false);
	    //std::vector<PartitionID> map_old_new(G_ref.number_of_nodes(),0);
	    //forall_nodes(G_ref, n) {
		    //PartitionID old_block = G_ref.getPartitionIndex(n);
		    //if (!used_block[old_block]) {
			    //used_block[old_block] = true;
			    //map_old_new[old_block] = k++;
		    //}
		    //PartitionID new_block = map_old_new[old_block];
		    //G_ref.setPartitionIndex(n, new_block);
	    //} endfor
	    //config.k = k;
	    //G_ref.set_partition_count(config.k);

	    //boundary = new complete_boundary(G);
	    //boundary->build();  // update boundary after remapping of cluster IDs as clusters may have disappeared
	    //overall_improvement += fm_local_search->perform_refinement(config, *G, *boundary);
	    //delete boundary;

	    //[> std::cout << "edge-cut QG: " << qm.edge_cut(*G) << "\n"; <]
    //}

    ////std::cout <<  "QR refinement " <<  t.elapsed()  << std::endl;
    //t.restart();


    // K-Way FM Local Search
    if (!config.disable_kway_fm) {
	    //int k = 0;
            //t.restart();
	    //graph_access & G_ref = *G;
	    //std::vector<bool> used_block(G_ref.number_of_nodes(),false);
	    //std::vector<PartitionID> map_old_new(G_ref.number_of_nodes(),0);
	    //forall_nodes(G_ref, n) {
		    //PartitionID old_block = G_ref.getPartitionIndex(n);
		    //if (!used_block[old_block]) {
			    //used_block[old_block] = true;
			    //map_old_new[old_block] = k++;
		    //}
		    //PartitionID new_block = map_old_new[old_block];
		    //G_ref.setPartitionIndex(n, new_block);
	    //} endfor
	    //config.k = k;
	    //G_ref.set_partition_count(config.k);
            //t.restart();

	    //boundary = new complete_boundary(G);
	    //boundary->build(); // update boundary after remapping of cluster IDs as clusters may have disappeared
            //std::cout <<  "building boundary took " <<  t.elapsed() << std::endl;
	    overall_improvement += kway->perform_refinement(config, *G);
	    //delete boundary;

	    /* std::cout << "edge-cut KW: " << qm.edge_cut(*G) << "\n"; */
    }
    //std::cout <<  "kFM refinement " <<  t.elapsed()  << std::endl;
    //t.restart();



    // Multi-Try K-Way FM Local Search
    //if (!config.disable_fm_multitry) {
            //int k = 0;
            //graph_access & G_ref = *G;
            //std::vector<bool> used_block(G_ref.number_of_nodes(),false);
            //std::vector<PartitionID> map_old_new(G_ref.number_of_nodes(),0);
            //forall_nodes(G_ref, n) {
                    //PartitionID old_block = G_ref.getPartitionIndex(n);
                    //if (!used_block[old_block]) {
                            //used_block[old_block] = true;
                            //map_old_new[old_block] = k++;
                    //}
                    //PartitionID new_block = map_old_new[old_block];
                    //G_ref.setPartitionIndex(n, new_block);
            //} endfor
            //config.k = k;
            //G_ref.set_partition_count(config.k);

            //boundary = new complete_boundary(G);
            //boundary->build(); // update boundary after remapping of cluster IDs as clusters may have disappeared
            //overall_improvement += multitry_kway->perform_refinement(config, *G, *boundary, config.local_multitry_rounds, true, config.local_multitry_fm_alpha);
            //delete boundary;

            ////[> std::cout << "edge-cut MT: " << qm.edge_cut(*G) << "\n"; <]
    //}
    //std::cout <<  "multiFM refinement " <<  t.elapsed()  << std::endl;
    //t.restart();



    return overall_improvement;
}
