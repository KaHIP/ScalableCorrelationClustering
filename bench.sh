#!/bin/bash
SCC="./deploy/scc"
GRAPH_DIR="/home/graph_collection/signed_graph_clustering"
TIMEOUT=600
SEEDS="0 1 2 3 4 5 6 7 8 9"

run_instance() {
    local inst="$1" seed="$2"
    local outfile="/tmp/scc_bench_${inst}_s${seed}.log"
    timeout "$TIMEOUT" "$SCC" "$GRAPH_DIR/$inst" --seed="$seed" > "$outfile" 2>&1
    if [ $? -ne 0 ]; then echo -e "${inst}\t${seed}\tCRASH\t0"; return; fi
    local time_val=$(grep "time spent for partitioning" "$outfile" | awk '{print $NF}')
    local cut_val=$(grep "^cut" "$outfile" | awk '{print $NF}')
    if [ -z "$time_val" ] || [ -z "$cut_val" ]; then echo -e "${inst}\t${seed}\tERROR\t0"; return; fi
    echo -e "${inst}\t${seed}\t${time_val}\t${cut_val}"
}
export -f run_instance
export SCC GRAPH_DIR TIMEOUT
INSTANCES=(
    "cityscape34.graph" "cityscape52.graph" "cityscape7.graph"
    "wikiconflict.graph" "wikisigned-k2.graph" "soc-sign-epinions.graph"
    "soc-sign-Slashdot090221.graph" "soc-sign-Slashdot090216.graph"
    "slashdot-zoo.graph" "soc-sign-Slashdot081106.graph"
)
echo "instance	seed	time	cut"
for inst in "${INSTANCES[@]}"; do
    for seed in $SEEDS; do echo "$inst $seed"; done
done | parallel --jobs $(nproc) --colsep ' ' run_instance {1} {2}
