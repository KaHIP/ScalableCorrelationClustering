#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <libgen.h>

static bool file_has_double_weights(const std::string & filename) {
        std::ifstream in(filename.c_str());
        if (!in) return false;

        std::string line;
        std::getline(in, line);
        while (!in.eof() && (line.empty() || line[0] == '%' || line[0] == '#')) {
                std::getline(in, line);
        }

        std::stringstream hdr(line);
        long nmbNodes, nmbEdges;
        int ew = 0;
        hdr >> nmbNodes >> nmbEdges >> ew;

        bool has_edge_weights = (ew == 1 || ew == 11);
        bool has_node_weights = (ew == 10 || ew == 11);

        auto is_float_token = [](const std::string & tok) -> bool {
                for (size_t i = 0; i < tok.size(); i++) {
                        char c = tok[i];
                        if (c == '.') return true;
                        if ((c == 'e' || c == 'E') && i > 0) return true;
                }
                return false;
        };

        int lines_checked = 0;
        while (lines_checked < 20 && std::getline(in, line)) {
                if (line.empty() || line[0] == '%' || line[0] == '#') continue;
                lines_checked++;

                std::stringstream ss(line);
                std::string tok;

                if (ew == 0) {
                        ss >> tok; ss >> tok;
                        if (ss >> tok) {
                                if (is_float_token(tok)) return true;
                        }
                } else {
                        if (has_node_weights) ss >> tok;
                        while (ss >> tok) {
                                if (has_edge_weights && (ss >> tok)) {
                                        if (is_float_token(tok)) return true;
                                }
                        }
                }
        }
        return false;
}

static std::string find_graph_file(int argc, char** argv) {
        for (int i = 1; i < argc; i++) {
                if (argv[i][0] == '-') continue;
                std::ifstream f(argv[i]);
                if (f.good()) return argv[i];
        }
        return "";
}

int main(int argc, char** argv) {
        std::string graph_file = find_graph_file(argc, argv);
        if (graph_file.empty()) {
                std::cerr << "Could not find graph file in arguments." << std::endl;
                return 1;
        }

        // resolve path to the directory containing this binary
        char self_path[4096];
        ssize_t len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
        std::string dir;
        if (len > 0) {
                self_path[len] = '\0';
                dir = std::string(dirname(self_path));
        } else {
                dir = ".";
        }

        bool use_double = file_has_double_weights(graph_file);
        std::string target = dir + (use_double ? "/scc_double" : "/scc_int");

        argv[0] = strdup(target.c_str());
        execvp(target.c_str(), argv);

        // execvp only returns on error
        perror("execvp failed");
        return 1;
}
