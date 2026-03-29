/******************************************************************************
 * graph_io.h
 * *
 * Source of KaHIP -- Karlsruhe High Quality Partitioning.
 * Christian Schulz <christian.schulz.phone@gmail.com>
 *****************************************************************************/

#ifndef GRAPHIO_H_
#define GRAPHIO_H_

#include <fstream>
#include <iostream>
#include <limits>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#include "definitions.h"
#include "data_structure/graph_access.h"
#include "tools/random_functions.h"


///////// Classes for processing logs
class LogRecord {
	public:
		LogRecord(double timestamp, EdgeWeight objective) {
			this->timestamp = timestamp;
			this->objective = objective;
		}
		double timestamp;
		EdgeWeight objective;
};
class LogVector {
	public:
		LogVector() {
			records = new std::vector<LogRecord>();
		}
		~LogVector() {
			delete records;
		}
		void push_back(double timestamp, EdgeWeight objective) {
			records->push_back(LogRecord(timestamp,objective));
		}
		void sort() {
			std::sort(records->begin(), records->end(), [] (const LogRecord &lhs, const LogRecord &rhs) {
				if (lhs.timestamp == rhs.timestamp)
					return lhs.objective < rhs.objective;
				return lhs.timestamp < rhs.timestamp;
			});
		}
		size_t size() const {
			return records->size();
		}
		double timestamp(int pos) const {
			return (*records)[pos].timestamp;
		}
		EdgeWeight objective(int pos) const {
			return (*records)[pos].objective;
		}
		void normalize_timestamp() {
			double unitary_value = (*records)[0].timestamp;
			for (int i=0; i<records->size(); i++) {
				(*records)[i].timestamp /= unitary_value;
			}
		}
	private:
		std::vector<LogRecord>* records;
};
///////// Classes for processing logs


class graph_io {
        public:
                graph_io();
                virtual ~graph_io () ;

                static
		int writeMap(std::vector<NodeID> &map_real_virtual, const std::string & filename);

                static
		int readWeightedEdgeStreamToGraph(graph_access & G, const std::string & filename, std::vector<NodeID> &map_real_virtual, bool relabel_nodes);

                static
                int readGraphWeighted(graph_access & G, const std::string & filename);

                static
                int writeRandomSignedGraphWeighted(graph_access & G, const std::string & filename);

                static
                int writeGraphWeighted(graph_access & G, const std::string & filename);

                static
                int writeGraph(graph_access & G, const std::string & filename);

                static
                int readPartition(graph_access& G, const std::string & filename);

                static
                void writePartition(graph_access& G, const std::string & filename);

                template<typename vectortype>
                static void writeVector(std::vector<vectortype> & vec, const std::string & filename);

                template<typename vectortype>
                static void readVector(std::vector<vectortype> & vec, const std::string & filename);


		///////// Methods for processing logs
		static
		int readLogFile(LogVector & input_log, const std::string & filename);

		static
		void writeLogFile(LogVector & output_log, const std::string & filename);
		///////// Methods for processing logs

                static
                bool file_has_double_weights(const std::string & filename);
};

template<typename vectortype>
void graph_io::writeVector(std::vector<vectortype> & vec, const std::string & filename) {
        std::ofstream f(filename.c_str());
        for( unsigned i = 0; i < vec.size(); ++i) {
                f << vec[i] <<  std::endl;
        }

        f.close();
}

template<typename vectortype>
void graph_io::readVector(std::vector<vectortype> & vec, const std::string & filename) {

        std::string line;

        // open file for reading
        std::ifstream in(filename.c_str());
        if (!in) {
                std::cerr << "Error opening vectorfile" << filename << std::endl;
                return;
        }

        unsigned pos = 0;
        std::getline(in, line);
        while( !in.eof() ) {
                if (line[0] == '%') { //Comment
                        continue;
                }

                vectortype value = (vectortype) atof(line.c_str());
                vec[pos++] = value;
                std::getline(in, line);
        }

        in.close();
}

#endif /*GRAPHIO_H_*/
