/******************************************************************************
 * vertex_moved_hashtable.h 
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#ifndef VMOVEDHT_4563r97820954
#define VMOVEDHT_4563r97820954

#include <unordered_map>
#include <vector>

#include "definitions.h"
#include "limits.h"

//struct compare_nodes {
        //bool operator()(const NodeID lhs, const NodeID rhs) const {
                //return (lhs == rhs);
        //}
//};


const NodeID NOT_MOVED = std::numeric_limits<NodeID>::max();
const NodeID NOT_QUEUED = std::numeric_limits<NodeID>::max()-1;
const NodeID MOVED = 0;

struct moved_index {
        NodeID index;
        moved_index() {
                index = NOT_MOVED;
        }
};

//struct hash_nodes {
        //size_t operator()(const NodeID idx) const {
                //return idx;
        //}
//};

//typedef std::unordered_map<const NodeID, moved_index, hash_nodes, compare_nodes> vertex_moved_hashtable;

class vertex_moved_hashtable {
        public:
                vertex_moved_hashtable(int size ) {
                        elements.resize(size); 
                        for( unsigned i = 0; i < elements.size(); i++) {
                                elements[i].index = NOT_QUEUED;
                        }
                };
                virtual ~vertex_moved_hashtable() {};       

                void reset() {
                        for( unsigned i = 0; i < to_reset.size(); i++) {
                                elements[to_reset[i]].index = NOT_QUEUED;
                        }
                        to_reset.clear();
                } ;

                void touch(unsigned index) {
                        to_reset.push_back(index);
                } ;

                moved_index& operator[](int index) {
                        return elements[index];
                };
        private:
                std::vector<moved_index> elements;
                std::vector<unsigned> to_reset;
};


#endif
