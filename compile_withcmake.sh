#!/bin/bash
set -e

NCORES=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Building SCC with ${NCORES} parallel jobs..."

rm -rf build deploy
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "${NCORES}"

mkdir -p deploy
cp build/scc \
   build/scc_evolutionary \
   build/scc_evaluator \
   build/scc_graphchecker deploy/

echo ""
echo "Done. Binaries are in ./deploy/"
echo "  scc                - multilevel signed graph clustering"
echo "  scc_evolutionary   - distributed memetic signed graph clustering (MPI)"
echo "  scc_evaluator      - evaluate a clustering"
echo "  scc_graphchecker   - check graph format"
