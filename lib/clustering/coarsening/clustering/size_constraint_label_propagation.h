/******************************************************************************
 * constraint_label_propagation.cpp 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#ifndef SIZE_CONSTRAINT_LABEL_PROPAGATION_7SVLBKKT
#define SIZE_CONSTRAINT_LABEL_PROPAGATION_7SVLBKKT

#include <unordered_map>
#include "data_structure/graph_access.h"
#include "partition_config.h"

struct ensemble_pair {
        PartitionID n; // number of nodes in the graph
        PartitionID lhs;
        PartitionID rhs;
};

struct compare_ensemble_pair {
        bool operator()(const ensemble_pair pair_a, const ensemble_pair pair_b) const {
                bool eq = (pair_a.lhs == pair_b.lhs && pair_a.rhs == pair_b.rhs);
                return eq;
        }
};

struct hash_ensemble_pair{
       size_t operator()(const ensemble_pair pair) const {
                return pair.lhs*pair.n + pair.rhs;
       }
};

struct data_ensemble_pair {
        NodeID mapping;

        data_ensemble_pair() {
                mapping = 0;
        }
};

typedef std::unordered_map<const ensemble_pair, 
                                data_ensemble_pair, 
                                hash_ensemble_pair, 
                                compare_ensemble_pair> hash_ensemble;


class size_constraint_label_propagation {
        public:
                size_constraint_label_propagation();
                virtual ~size_constraint_label_propagation();

        private:
                // Persistent buffers reused across calls to avoid repeated allocation
                std::vector<EdgeWeight> m_hash_map;
                std::vector<NodeID> m_permutation;
                std::vector<char> m_qc_a, m_qc_b;

        public:

                void match(const PartitionConfig & config, 
                                graph_access & G,
                                CoarseMapping & coarse_mapping, 
                                NodeID & no_of_coarse_vertices,
                                NodeID & labels_changed);


                void ensemble_clusterings(const PartitionConfig & config, 
                                graph_access & G,
                                CoarseMapping & coarse_mapping, 
                                NodeID & no_of_coarse_vertices,
                                NodeID & labels_changed);

                void ensemble_two_clusterings( graph_access & G,
                                std::vector<NodeID> & lhs, 
                                std::vector<NodeID> & rhs, 
                                std::vector< NodeID > & output,
                                NodeID & no_of_coarse_vertices);

                void match_internal(const PartitionConfig & config, 
                                graph_access & G,
                                CoarseMapping & coarse_mapping, 
                                NodeID & no_of_coarse_vertices,
                                NodeID & labels_changed);

                void label_propagation(const PartitionConfig & partition_config,
                               graph_access & G,
                               std::vector<NodeID> & cluster_id, // output parameter
                               NodeID & number_of_blocks,
                               NodeID & labels_changed); // output parameter

                void remap_cluster_ids(graph_access & G,
                                std::vector<NodeID> & cluster_id, 
                                NodeID & no_of_coarse_vertices,
                                bool apply_to_graph = false); 

                void create_coarsemapping(graph_access & G,
                                std::vector<NodeID> & cluster_id, 
                                CoarseMapping & coarse_mapping);
};


#endif /* end of include guard: SIZE_CONSTRAINT_LABEL_PROPAGATION_7SVLBKKT */
