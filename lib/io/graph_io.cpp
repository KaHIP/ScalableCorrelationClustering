/******************************************************************************
 * graph_io.cpp
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <unordered_map>
#include <set>
#include "graph_io.h"

graph_io::graph_io() {

}

graph_io::~graph_io() {

}

int graph_io::readGraphWeighted(graph_access & G, const std::string & filename) {
        std::string line;

        // open file for reading
        std::ifstream in(filename.c_str());
        if (!in) {
                std::cerr << "Error opening " << filename << std::endl;
                return 1;
        }

        long nmbNodes;
        long nmbEdges;

        std::getline(in,line);
        //skip comments
        while( line[0] == '%' ) {
                std::getline(in, line);
        }

        int ew = 0;
        std::stringstream ss(line);
        ss >> nmbNodes;
        ss >> nmbEdges;
        ss >> ew;

        if( 2*nmbEdges > std::numeric_limits<int>::max() || nmbNodes > std::numeric_limits<int>::max()) {
                std::cerr <<  "The graph is too large. Currently only 32bit supported!"  << std::endl;
                exit(0);
        }

        bool read_ew = false;
        bool read_nw = false;

        if(ew == 1) {
                read_ew = true;
        } else if (ew == 11) {
                read_ew = true;
                read_nw = true;
        } else if (ew == 10) {
                read_nw = true;
        }
        nmbEdges *= 2; //since we have forward and backward edges

        NodeID node_counter   = 0;
        EdgeID edge_counter   = 0;
        uint64_t total_nodeweight = 0;

        G.start_construction(nmbNodes, nmbEdges);

        while(  std::getline(in, line)) {

                if (line[0] == '%') { // a comment in the file
                        continue;
                }

                NodeID node = G.new_node(); node_counter++;
                G.setPartitionIndex(node, 0);

                std::stringstream ss(line);

                NodeWeight weight = 1;
                if( read_nw ) {
                        ss >> weight;
                        total_nodeweight += (uint64_t)weight;
                        if( total_nodeweight > (uint64_t) std::numeric_limits<NodeWeight>::max()) {
                                std::cerr <<  "The sum of the node weights is too large (it exceeds the node weight type)."  << std::endl;
                                std::cerr <<  "Currently not supported. Please scale your node weights."  << std::endl;
                                exit(0);
                        }
                }
                G.setNodeWeight(node, weight);

                NodeID target;
                while( ss >> target ) {
                        //check for self-loops
                        if(target-1 == node) {
                                std::cerr <<  "The graph file contains self-loops. This is not supported. Please remove them from the file."  << std::endl;
                        }

                        EdgeWeight edge_weight = 1;
                        if( read_ew ) {
                                ss >> edge_weight;
                        }
                        edge_counter++;
                        EdgeID e = G.new_edge(node, target-1);

                        G.setEdgeWeight(e, edge_weight);
                }

                if(in.eof()) {
                        break;
                }
        }

        if( edge_counter != (EdgeID) nmbEdges ) {
                std::cerr <<  "number of specified edges mismatch"  << std::endl;
                std::cerr <<  edge_counter <<  " " <<  nmbEdges  << std::endl;
                exit(0);
        }

        if( node_counter != (NodeID) nmbNodes) {
                std::cerr <<  "number of specified nodes mismatch"  << std::endl;
                std::cerr <<  node_counter <<  " " <<  nmbNodes  << std::endl;
                exit(0);
        }


        G.finish_construction();
        return 0;
}

int graph_io::writeMap(std::vector<NodeID> &map_real_virtual, const std::string & filename) {
        std::ofstream f(filename.c_str());
	for (auto& i: map_real_virtual) {
		f << i << "\n" ;
	}
        f.close();
        return 0;
}

int graph_io::readWeightedEdgeStreamToGraph(graph_access & G, const std::string & filename, std::vector<NodeID> &map_real_virtual, bool relabel_nodes) {
        std::string line;
        long nmbNodes;
        long nmbEdges;
	NodeID i, x, y;
	EdgeWeight ew;
	EdgeID num_edges = 0;
	NodeID num_nodes = 0;
	NodeID real_x, real_y;

        // open file for reading
        std::ifstream in(filename.c_str());
        if (!in) {
                std::cerr << "Error opening " << filename << std::endl;
                return 1;
        }

        std::getline(in,line);
        //skip comments
        while( line[0] == '%' || line[0] == '#' ) {
                std::getline(in, line);
        }

        std::stringstream ss(line);
        ss >> nmbNodes;
        ss >> nmbEdges;

	std::unordered_map<NodeID,NodeID> map_virtual_real;
	map_real_virtual.clear();

	std::vector<std::unordered_map<NodeID,EdgeWeight>> neighbors;
	neighbors.resize(nmbNodes);
        while(  std::getline(in, line)) {
                if (line[0] == '%' || line[0] == '#' ) {
                        continue;
                }
		std::stringstream ss(line);
		ss >> x;
		ss >> y;
		ss >> ew;

		if (x == y) {
			continue;
		}

		if (relabel_nodes) {
			if (map_virtual_real.find(x) == map_virtual_real.end()) {
				map_virtual_real[x] = num_nodes++;
				map_real_virtual.push_back(x);
			}
			if (map_virtual_real.find(y) == map_virtual_real.end()) {
				map_virtual_real[y] = num_nodes++;
				map_real_virtual.push_back(y);
			}
			real_x = map_virtual_real[x];
			real_y = map_virtual_real[y];
		} else {
			real_x = x;
			real_y = y;
			if (real_x >= nmbNodes || real_y >= nmbNodes) {
				std::cerr <<  "Nodes need to be relabeled for this input."  << std::endl;
				exit(0);
			}
		}	

		// Substitute arcs and parallel edges by a single undirected edge whose weight equals the sum of arc weights
#ifdef EDGE_WEIGHT_DOUBLE
		auto is_zero = [](EdgeWeight w) { return std::abs(w) < 1e-9; };
#else
		auto is_zero = [](EdgeWeight w) { return w == 0; };
#endif
		if (neighbors[real_x].find(real_y) == neighbors[real_x].end() || is_zero(neighbors[real_x][real_y])) {
			neighbors[real_x][real_y] = ew;
			num_edges++;
		} else {
			neighbors[real_x][real_y] += ew;
			if (is_zero(neighbors[real_x][real_y])) num_edges--;
		}

		if (neighbors[real_y].find(real_x) == neighbors[real_y].end() || is_zero(neighbors[real_y][real_x])) {
			neighbors[real_y][real_x] = ew;
			num_edges++;
		} else {
			neighbors[real_y][real_x] += ew;
			if (is_zero(neighbors[real_y][real_x])) num_edges--;
		}
	}

	nmbEdges = num_edges;
        if( nmbEdges > std::numeric_limits<int>::max() || nmbNodes > std::numeric_limits<int>::max() ) {
                std::cerr <<  "The graph is too large. Currently only 32bit supported!"  << std::endl;
                exit(0);
        }
	
	G.start_construction(nmbNodes, nmbEdges);
	for( i = 0 ; i < nmbNodes ; i++) {
                NodeID node = G.new_node(); 
                G.setPartitionIndex(node, 0);
                NodeWeight weight = 1;
                G.setNodeWeight(node, weight);
		for (auto& it : neighbors[node]) {
			NodeID target = it.first;
                        EdgeWeight edge_weight = it.second;
#ifdef EDGE_WEIGHT_DOUBLE
			if (std::abs(edge_weight) < 1e-9) continue;
#else
			if (edge_weight == 0) continue;
#endif
                        EdgeID e = G.new_edge(node, target);
                        G.setEdgeWeight(e, edge_weight);
                }
        }
        G.finish_construction();
	return 0;
}

int graph_io::writeGraph(graph_access & G, const std::string & filename) {
        std::ofstream f(filename.c_str());
        f << G.number_of_nodes() <<  " " <<  G.number_of_edges()/2 << std::endl;

        forall_nodes(G, node) {
                                forall_out_edges(G, e, node) {
                                                        f <<   (G.getEdgeTarget(e)+1) << " " ;
                                                } endfor
                                f <<  "\n";
                        } endfor

        f.close();
        return 0;
}

int graph_io::writeGraphWeighted(graph_access & G, const std::string & filename) {
        std::ofstream f(filename.c_str());
#ifdef EDGE_WEIGHT_DOUBLE
        f << std::setprecision(17);
#endif
        f << G.number_of_nodes() <<  " " <<  G.number_of_edges()/2 <<  " 11" <<  std::endl;

        forall_nodes(G, node) {
                f <<  G.getNodeWeight(node) ;
                forall_out_edges(G, e, node) {
                        f << " " <<   (G.getEdgeTarget(e)+1) <<  " " <<  G.getEdgeWeight(e) ;
                } endfor
                f <<  "\n";
        } endfor

        f.close();
        return 0;
}

int graph_io::writeRandomSignedGraphWeighted(graph_access & G, const std::string & filename) {
        std::ofstream f(filename.c_str());
        f << G.number_of_nodes() <<  " " <<  G.number_of_edges()/2 <<  " 11" <<  std::endl;
        std::map<std::pair<NodeID,NodeID>,EdgeWeight> negative;
        EdgeWeight signal;

        forall_nodes(G, node) {
                                f <<  G.getNodeWeight(node) ;
                                forall_out_edges(G, e, node) {
                                                        if (node<G.getEdgeTarget(e)) {
                                                                negative[std::make_pair(node,G.getEdgeTarget(e))] = (random_functions::nextBool()?-1:1);
                                                        }
                                                        signal = negative[std::make_pair(G.getEdgeTarget(e),node)];
                                                        f << " " << (G.getEdgeTarget(e)+1) << " " << (signal*G.getEdgeWeight(e));
                                                } endfor
                                f << "\n";
                        } endfor

        f.close();
        return 0;
}

int graph_io::readPartition(graph_access & G, const std::string & filename) {
        std::string line;

        // open file for reading
        std::ifstream in(filename.c_str());
        if (!in) {
                std::cerr << "Error opening file" << filename << std::endl;
                return 1;
        }

        PartitionID max = 0;
        forall_nodes(G, node) {
                                // fetch current line
                                std::getline(in, line);
                                if (line[0] == '%') { //Comment
                                        node--;
                                        continue;
                                }

                                // in this line we find the block of Node node
                                G.setPartitionIndex(node, (PartitionID) atol(line.c_str()));

                                if(G.getPartitionIndex(node) > max)
                                        max = G.getPartitionIndex(node);
                        } endfor

        G.set_partition_count(max+1);
        in.close();

        return 0;
}

void graph_io::writePartition(graph_access & G, const std::string & filename) {
        std::ofstream f(filename.c_str());
        std::cout << "writing partition to " << filename << " ... " << std::endl;

        forall_nodes(G, node) {
                f << G.getPartitionIndex(node) <<  "\n";
        } endfor

        f.close();
}




int graph_io::readLogFile(LogVector & input_log, const std::string & filename) {
        std::string line;
	double timestamp;
	EdgeWeight objective;

        // open file for reading
        std::ifstream in(filename.c_str());
        if (!in) {
                std::cerr << "Error opening " << filename << std::endl;
                return 1;
        }

        while (std::getline(in, line)) {
                if (line[0] == '%') { // a comment in the file
                        continue;
                }
                std::stringstream ss(line);
		ss >> timestamp >> objective;
		input_log.push_back(timestamp, objective);
                if(in.eof()) {
                        break;
                }
        }
        return 0;
}

void graph_io::writeLogFile(LogVector & output_log, const std::string & filename) {
        std::ofstream f(filename.c_str());

	for (int pos=0; pos<output_log.size(); pos++) {
                f << output_log.timestamp(pos) << " " << output_log.objective(pos) <<  "\n";
        }

        f.close();
}

bool graph_io::file_has_double_weights(const std::string & filename) {
        std::ifstream in(filename.c_str());
        if (!in) return false;

        std::string line;

        // skip comment lines
        std::getline(in, line);
        while (!in.eof() && (line.empty() || line[0] == '%' || line[0] == '#')) {
                std::getline(in, line);
        }

        // parse header: nodes edges [format]
        std::stringstream hdr(line);
        long nmbNodes, nmbEdges;
        int ew = 0;
        hdr >> nmbNodes >> nmbEdges >> ew;

        bool has_edge_weights = (ew == 1 || ew == 11);
        bool has_node_weights = (ew == 10 || ew == 11);

        // helper: check if a string token looks like a floating-point number
        auto is_float_token = [](const std::string & tok) -> bool {
                for (size_t i = 0; i < tok.size(); i++) {
                        char c = tok[i];
                        if (c == '.') return true;
                        if ((c == 'e' || c == 'E') && i > 0) return true;
                }
                return false;
        };

        // scan up to 20 data lines
        int lines_checked = 0;
        while (lines_checked < 20 && std::getline(in, line)) {
                if (line.empty() || line[0] == '%' || line[0] == '#') continue;
                lines_checked++;

                std::stringstream ss(line);
                std::string tok;

                if (ew == 0) {
                        // edge stream format: src dst weight
                        // first two tokens are node IDs, third is weight
                        ss >> tok; // src
                        ss >> tok; // dst
                        if (ss >> tok) {
                                if (is_float_token(tok)) return true;
                        }
                } else {
                        // METIS format: [nodeweight] target weight target weight ...
                        if (has_node_weights) {
                                ss >> tok; // skip node weight
                        }
                        while (ss >> tok) { // target
                                if (has_edge_weights && (ss >> tok)) {
                                        if (is_float_token(tok)) return true;
                                }
                        }
                }
        }

        return false;
}

