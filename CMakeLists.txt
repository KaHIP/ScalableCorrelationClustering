cmake_minimum_required(VERSION 3.10)
include(CheckCXXCompilerFlag)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  message(STATUS "Using compiler cache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
endif()
project(KaHIP C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g -O1")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# if no build mode is specified build in release mode
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# --march=nativeflag
option(NONATIVEOPTIMIZATIONS "Disable --march=native optimizations" OFF)
add_definitions(-Wno-stringop-overflow -fpermissive -O3 )
  #add_definitions(-g -pg  )
# tweak compiler flags
CHECK_CXX_COMPILER_FLAG(-funroll-loops COMPILER_SUPPORTS_FUNROLL_LOOPS)
if(COMPILER_SUPPORTS_FUNROLL_LOOPS)
  add_definitions(-funroll-loops)
endif()
CHECK_CXX_COMPILER_FLAG(-fno-stack-limit COMPILER_SUPPORTS_FNOSTACKLIMITS)
if(COMPILER_SUPPORTS_FNOSTACKLIMITS)
  add_definitions(-fno-stack-limit)
endif()
CHECK_CXX_COMPILER_FLAG(-Wall COMPILER_SUPPORTS_WALL)
if(COMPILER_SUPPORTS_WALL)
  add_definitions(-Wall)
endif()
CHECK_CXX_COMPILER_FLAG(-march=native COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE AND NOT NONATIVEOPTIMIZATIONS)
          add_definitions(-march=native)
endif()
CHECK_CXX_COMPILER_FLAG(-fpermissive COMPILER_SUPPORTS_FPERMISSIVE)
if(COMPILER_SUPPORTS_FPERMISSIVE)
  add_definitions(-fpermissive)
endif()
CHECK_CXX_COMPILER_FLAG(-Wno-unused-result COMPILER_SUPPORTS_UNUSED)
if(COMPILER_SUPPORTS_UNUSED)
  add_definitions(-Wno-unused-result)
endif()
CHECK_CXX_COMPILER_FLAG(-Wno-sign-compare COMPILER_SUPPORTS_NOSIGNCOMP)
if(COMPILER_SUPPORTS_NOSIGNCOMP)
  add_definitions(-Wno-sign-compare)
endif()

# check dependencies
find_package(MPI REQUIRED)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  message(STATUS "OpenMP support detected")
  add_definitions(${OpenMP_CXX_FLAGS})
else()
  message(WARNING "OpenMP not available, activating workaround")
  add_library(OpenMP::OpenMP_CXX IMPORTED INTERFACE)
  set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_COMPILE_OPTIONS "")
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/misc)
endif()

# optimized output
option(OPTIMIZED_OUTPUT "optimized output" OFF)
if(OPTIMIZED_OUTPUT)
  add_definitions("-DKAFFPAOUTPUT")
endif()

# tcmalloc
option(USE_TCMALLOC "if available, link against tcmalloc" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/app)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern/argtable3-3.0.3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extern/andres_graph/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/io)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/partition)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/partition/uncoarsening/refinement/quotient_graph_refinement/flow_refinement)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/tools)

set(LIBKAFFPA_SOURCE_FILES
  lib/data_structure/graph_hierarchy.cpp
  lib/algorithms/strongly_connected_components.cpp
  lib/algorithms/topological_sort.cpp
  lib/algorithms/push_relabel.cpp
  lib/io/graph_io.cpp
  lib/tools/quality_metrics.cpp
  lib/tools/random_functions.cpp
  lib/tools/graph_extractor.cpp
  lib/tools/misc.cpp
  lib/tools/partition_snapshooter.cpp
  lib/partition/graph_partitioner.cpp
  lib/partition/w_cycles/wcycle_partitioner.cpp
  lib/partition/coarsening/coarsening.cpp
  lib/partition/coarsening/contraction.cpp
  lib/partition/coarsening/edge_rating/edge_ratings.cpp
  lib/partition/coarsening/matching/matching.cpp
  lib/partition/coarsening/matching/random_matching.cpp
  lib/partition/coarsening/matching/gpa/path.cpp
  lib/partition/coarsening/matching/gpa/gpa_matching.cpp
  lib/partition/coarsening/matching/gpa/path_set.cpp
  lib/partition/coarsening/clustering/node_ordering.cpp
  lib/partition/coarsening/clustering/size_constraint_label_propagation.cpp
  lib/partition/initial_partitioning/initial_partitioning.cpp
  lib/partition/initial_partitioning/initial_partitioner.cpp
  lib/partition/initial_partitioning/initial_partition_bipartition.cpp
  lib/partition/initial_partitioning/initial_refinement/initial_refinement.cpp
  lib/partition/initial_partitioning/bipartition.cpp
  lib/partition/initial_partitioning/initial_node_separator.cpp
  lib/partition/uncoarsening/uncoarsening.cpp
  lib/partition/uncoarsening/separator/area_bfs.cpp
  lib/partition/uncoarsening/separator/vertex_separator_algorithm.cpp
  lib/partition/uncoarsening/separator/vertex_separator_flow_solver.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/greedy_neg_cycle.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/problem_factory.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/augmented_Qgraph.cpp
  lib/partition/uncoarsening/refinement/mixed_refinement.cpp
  lib/partition/uncoarsening/refinement/label_propagation_refinement/label_propagation_refinement.cpp
  lib/partition/uncoarsening/refinement/refinement.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/2way_fm_refinement/two_way_fm.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/flow_refinement/two_way_flow_refinement.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/flow_refinement/boundary_bfs.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/flow_refinement/flow_solving_kernel/cut_flow_problem_solver.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/flow_refinement/most_balanced_minimum_cuts/most_balanced_minimum_cuts.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/quotient_graph_refinement.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/complete_boundary.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/partial_boundary.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/quotient_graph_scheduling/quotient_graph_scheduling.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/quotient_graph_scheduling/simple_quotient_graph_scheduler.cpp
  lib/partition/uncoarsening/refinement/quotient_graph_refinement/quotient_graph_scheduling/active_block_quotient_graph_scheduler.cpp
  lib/partition/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement.cpp
  lib/partition/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement_core.cpp
  lib/partition/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement_commons.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/augmented_Qgraph_fabric.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/advanced_models.cpp
  lib/partition/uncoarsening/refinement/kway_graph_refinement/multitry_kway_fm.cpp
  lib/partition/uncoarsening/refinement/node_separators/greedy_ns_local_search.cpp
  lib/partition/uncoarsening/refinement/node_separators/fm_ns_local_search.cpp
  lib/partition/uncoarsening/refinement/node_separators/localized_fm_ns_local_search.cpp
  lib/algorithms/cycle_search.cpp
  lib/partition/uncoarsening/refinement/cycle_improvements/cycle_refinement.cpp
  lib/partition/uncoarsening/refinement/tabu_search/tabu_search.cpp
  extern/argtable3-3.0.3/argtable3.c
  tools/tools.cpp)
add_library(libkaffpa OBJECT ${LIBKAFFPA_SOURCE_FILES})


set(LIBCLUSTERING_SOURCE_FILES
  extern/argtable3-3.0.3/argtable3.c
  lib/clustering/signed_graph_clusterer.cpp
  lib/clustering/coarsening/clustering/node_ordering.cpp
  lib/clustering/coarsening/clustering/size_constraint_label_propagation.cpp
  lib/clustering/coarsening/coarsening.cpp
  lib/clustering/coarsening/contraction.cpp
  lib/tools/graph_extractor.cpp
  lib/clustering/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement.cpp
  lib/clustering/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement_commons.cpp
  lib/clustering/uncoarsening/refinement/kway_graph_refinement/kway_graph_refinement_core.cpp
  lib/clustering/uncoarsening/refinement/label_propagation_refinement/label_propagation_refinement.cpp
  lib/clustering/uncoarsening/refinement/quotient_graph_refinement/complete_boundary.cpp
  lib/clustering/uncoarsening/refinement/quotient_graph_refinement/partial_boundary.cpp
  lib/clustering/uncoarsening/refinement/refinement.cpp
  lib/clustering/uncoarsening/uncoarsening.cpp
  lib/data_structure/graph_hierarchy.cpp
  lib/io/graph_io.cpp
  lib/tools/quality_metrics.cpp
  lib/tools/random_functions.cpp
  tools/tools.cpp)
add_library(libclustering OBJECT ${LIBCLUSTERING_SOURCE_FILES})

set(LIBCLUSTERING_EVOLUTIONARY_SOURCE_FILES
  lib/algorithms/cycle_search.cpp
  lib/algorithms/strongly_connected_components.cpp
  lib/clustering_evolutionary/evolutionary_signed_graph_clusterer.cpp
  lib/clustering_evolutionary/population.cpp
  lib/clustering_evolutionary/exchange/exchanger.cpp
  lib/tools/graph_communication.cpp
  lib/tools/mpi_tools.cpp)
add_library(libclustering_evolutionary OBJECT ${LIBCLUSTERING_EVOLUTIONARY_SOURCE_FILES})
target_include_directories(libclustering_evolutionary PUBLIC ${MPI_CXX_INCLUDE_PATH})

add_executable(scc_evaluator app/evaluator.cpp $<TARGET_OBJECTS:libkaffpa> )
target_compile_definitions(scc_evaluator PRIVATE "-DMODE_EVALUATOR")
target_link_libraries(scc_evaluator ${OpenMP_CXX_LIBRARIES})
install(TARGETS scc_evaluator DESTINATION bin)

add_executable(scc_graphchecker app/graphchecker.cpp $<TARGET_OBJECTS:libkaffpa> )
target_compile_definitions(scc_graphchecker PRIVATE "-DMODE_GRAPHCHECKER")
target_link_libraries(scc_graphchecker ${OpenMP_CXX_LIBRARIES})
install(TARGETS scc_graphchecker DESTINATION bin)


add_executable(scc app/signed_graph_clustering.cpp $<TARGET_OBJECTS:libclustering>)
target_compile_definitions(scc PRIVATE "-DMODE_CLUSTERING")
target_include_directories(scc PUBLIC ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(scc ${MPI_CXX_LIBRARIES} ${OpenMP_CXX_LIBRARIES})
install(TARGETS scc DESTINATION bin)

add_executable(scc_evolutionary app/signed_graph_clustering_evolutionary.cpp $<TARGET_OBJECTS:libclustering> $<TARGET_OBJECTS:libclustering_evolutionary>)
target_compile_definitions(scc_evolutionary PRIVATE "-DMODE_CLUSTERING_EVOLUTIONARY")
target_include_directories(scc_evolutionary PUBLIC ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(scc_evolutionary ${MPI_CXX_LIBRARIES} ${OpenMP_CXX_LIBRARIES})
install(TARGETS scc_evolutionary DESTINATION bin)

