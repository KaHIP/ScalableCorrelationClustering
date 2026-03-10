SCC v1.0
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-11-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C.svg)](https://cmake.org/)
[![GitHub Release](https://img.shields.io/github/v/release/KaHIP/ScalableCorrelationClustering)](https://github.com/KaHIP/ScalableCorrelationClustering/releases/latest)
[![Linux](https://img.shields.io/badge/Linux-supported-success.svg)](https://github.com/KaHIP/ScalableCorrelationClustering)
[![macOS](https://img.shields.io/badge/macOS-supported-success.svg)](https://github.com/KaHIP/ScalableCorrelationClustering)
[![GitHub Stars](https://img.shields.io/github/stars/KaHIP/ScalableCorrelationClustering)](https://github.com/KaHIP/ScalableCorrelationClustering/stargazers)
[![GitHub Issues](https://img.shields.io/github/issues/KaHIP/ScalableCorrelationClustering)](https://github.com/KaHIP/ScalableCorrelationClustering/issues)
[![Last Commit](https://img.shields.io/github/last-commit/KaHIP/ScalableCorrelationClustering)](https://github.com/KaHIP/ScalableCorrelationClustering/commits)
[![arXiv](https://img.shields.io/badge/arXiv-2208.13618-b31b1b.svg)](https://arxiv.org/abs/2208.13618)
[![ALENEX 2025](https://img.shields.io/badge/ALENEX'25-10.1137/1.9781611978339.7-blue)](https://doi.org/10.1137/1.9781611978339.7)
[![Heidelberg University](https://img.shields.io/badge/Heidelberg-University-c1002a)](https://www.uni-heidelberg.de)
=====

<p align="center">
  <img src="https://raw.githubusercontent.com/KaHIP/ScalableCorrelationClustering/main/logo/scc-logo.png" alt="SCC Logo" width="900"/>
</p>

**SCC** (Scalable Correlation Clustering) is a framework for clustering signed graphs, where edges carry positive (attraction) or negative (repulsion) weights. Part of the [KaHIP](https://github.com/KaHIP) organization.

| | |
|:--|:--|
| **What it solves** | Cluster signed graphs by grouping nodes with positive connections while separating nodes with negative connections |
| **Objective** | Minimize the sum of positive inter-cluster edges and negative intra-cluster edges |
| **Algorithms** | Multilevel approach with label propagation and local search; distributed memetic algorithm combining evolutionary search with multilevel refinement |
| **Interfaces** | CLI |
| **Requires** | C++11 compiler, CMake 3.10+, MPI (e.g. OpenMPI) |

## Quick Start

### Install via Homebrew

```bash
brew install KaHIP/kahip/scc
```

### Or build from source

```bash
git clone https://github.com/KaHIP/ScalableCorrelationClustering.git && cd ScalableCorrelationClustering
./compile_withcmake.sh
```

### Run

```bash
# Multilevel clustering
scc network.graph --seed=0

# Distributed memetic algorithm (4 MPI processes, 120s time limit)
mpirun -n 4 scc_evolutionary network.graph --seed=0 --time_limit=120
```

When building from source, binaries are in `./deploy/` (use `./deploy/scc` etc.).

---

## Executables

| Binary | Description |
|:-------|:------------|
| `scc` | Multilevel signed graph clustering |
| `scc_evolutionary` | Distributed memetic signed graph clustering (MPI) |
| `scc_evaluator` | Evaluate a clustering solution |
| `scc_graphchecker` | Check graph format validity |

---

## Command Line Usage

```
scc <graph-file> [options]
mpirun -n <procs> scc_evolutionary <graph-file> [options]
```

| Option | Description | Default |
|:-------|:-----------|:--------|
| `--seed=<int>` | Random seed for the PRNG | `0` |
| `--time_limit=<double>` | Time limit in seconds (0 = single run) | `0` |
| `--output_filename=<string>` | Output file for the clustering | `clustering` |
| `--help` | Print help | |

---

## Graph Format

SCC uses the standard **METIS graph format** with edge weights (format flag `1`). Positive edge weights represent attraction, negative weights represent repulsion.

**Header line:**
```
n m 1
```
- `n` = number of vertices, `m` = number of undirected edges, `1` = edge-weighted

**Vertex lines (one per vertex):**
Each of the following `n` lines lists pairs of `neighbor weight`:

```
v_1 w_1 v_2 w_2 ... v_k w_k
```

Vertices are **1-indexed**. Positive weights indicate attraction (+), negative weights indicate repulsion (-).

**Example** (4 vertices, 5 edges):
```
4 5 1
2 1 3 1
1 1 3 -1 4 1
1 1 2 -1 4 -1
2 1 3 -1
```

Vertices 1-2 and 2-4 are attracted (+1), while edges 2-3 and 3-4 are repulsive (-1). The optimal clustering places {1, 2, 4} in one cluster and {3} in another.

---

## Building from Source

### Requirements

- C++11 compiler (GCC 7+ or Clang 11+)
- CMake 3.10+
- MPI (e.g. OpenMPI)

### Build

```bash
./compile_withcmake.sh
```

Or using CMake directly:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Binaries are placed in `./build/` (CMake) or `./deploy/` (compile script).

---

## How It Works

SCC provides two algorithms for signed graph clustering:

1. **Multilevel algorithm** (`scc`): Coarsens the graph through label propagation, computes an initial clustering, then uncoarsens with FM-based local search refinement at each level.

2. **Distributed memetic algorithm** (`scc_evolutionary`): Maintains a population of clustering solutions across MPI processes. Individuals are evolved through mutation (re-clustering with different seeds) and crossover (combining two clusterings via the multilevel framework), with the best solutions surviving across generations.

Both algorithms automatically determine the number of clusters.

---

## Related Projects

| Project | Description |
|:--------|:------------|
| [KaHIP](https://github.com/KaHIP/KaHIP) | Karlsruhe High Quality Graph Partitioning (flagship framework) |
| [VieClus](https://github.com/KaHIP/VieClus) | Graph clustering optimizing modularity |
| [CluStRE](https://github.com/KaHIP/CluStRE) | Streaming graph clustering with multi-stage refinement |

---

## Licence

SCC is free software provided under the MIT License.
If you publish results using our algorithms, please acknowledge our work by citing our paper:

```
@InProceedings{HausbergerFFS25,
  author    = {Felix Hausberger and Marcelo Fonseca Faraj and Christian Schulz},
  title     = {{Scalable Multilevel and Memetic Signed Graph Clustering}},
  booktitle = {Proceedings of the 27th Symposium on Algorithm Engineering and Experiments (ALENEX 2025)},
  pages     = {81--94},
  publisher = {SIAM},
  year      = {2025},
  doi       = {10.1137/1.9781611978339.7}
}
```
