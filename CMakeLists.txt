
cmake_minimum_required(VERSION 3.22)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")

project(reprompi
        VERSION 1.0.0
        LANGUAGES C CXX)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(MPI REQUIRED)

find_package(GSL REQUIRED)
if (GSL_INCLUDE_DIRS)
message (STATUS "GSL INCLUDES: ${GSL_INCLUDE_DIRS}")
else(GSL_INCLUDE_DIRS)
message (FATAL_ERROR "GSL libraries not found.")
endif(GSL_INCLUDE_DIRS)


INCLUDE_DIRECTORIES(${GSL_INCLUDE_DIR} "src")
set(COMMON_LIBRARIES ${GSL_LIBRARIES} )

option(ENABLE_DOUBLE_BARRIER "Call barrier twice for synchronization [default: disabled]" off)

set(SET_BUFFER_ALIGNMENT "" CACHE STRING "Alignment to use for buffer allocation [default: no alignment(allocation with calloc)]")
option(SET_PRINT_MSIZES_BYTES "Print message sizes in the output instead of MPI call counts [default: counts]" off)

option(ENABLE_LOGGING   "Enable logging" off)
option(ENABLE_DEBUGGING "Enable debugging" off)
option(ENABLE_PGCHECKER "Enable performance guidelines checking" off)
option(ENABLE_BENCH_TESTS "Enable benchmark testing [default: disabled]" off)

SET(MY_COMPILE_FLAGS "")

######################################
############ Set timing flags ########
######################################
if(ENABLE_GETTIME_REALTIME)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DENABLE_GETTIME_REALTIME")
endif()

if(ENABLE_GETTIME_MONOTONIC)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DENABLE_GETTIME_MONOTONIC")
endif()

#################################
############ Other flags ########
#################################
if(SET_BUFFER_ALIGNMENT)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DSET_BUFFER_ALIGNMENT=${SET_BUFFER_ALIGNMENT}")
endif()

if(SET_PRINT_MSIZES_BYTES)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -DSET_PRINT_MSIZES_BYTES=${SET_PRINT_MSIZES_BYTES}")
endif()

if(ENABLE_DEBUGGING)
    SET(MY_COMPILE_FLAGS "${MY_COMPILE_FLAGS} -g")
endif()


set(COLL_OPS_SRC_FILES
${SRC_DIR}/collective_ops/collectives.c
${SRC_DIR}/collective_ops/mpi_collectives.c
${SRC_DIR}/collective_ops/pingpong.c
# memory allocation
${SRC_DIR}/buf_manager/mem_allocation.c
)


set(COMMON_OPTION_PARSER_SRC_FILES
${SRC_DIR}/reprompi_bench/option_parser/parse_common_options.c
${SRC_DIR}/reprompi_bench/option_parser/option_parser_helpers.c
)


set(SYNC_SRC_FILES
${SRC_DIR}/reprompi_bench/sync/process_sync/barrier_sync.c
${SRC_DIR}/reprompi_bench/sync/process_sync/barrier_sync_common.c
${SRC_DIR}/reprompi_bench/sync/process_sync/dissem_barrier_impl.c
${SRC_DIR}/reprompi_bench/sync/process_sync/dissem_barrier_sync.c
${SRC_DIR}/reprompi_bench/sync/process_sync/double_barrier_sync.c
${SRC_DIR}/reprompi_bench/sync/process_sync/proc_sync_none.c
${SRC_DIR}/reprompi_bench/sync/process_sync/round_time_sync.c
${SRC_DIR}/reprompi_bench/sync/process_sync/round_sync_common.c
${SRC_DIR}/reprompi_bench/sync/process_sync/reprompi_collectives.c
${SRC_DIR}/reprompi_bench/sync/process_sync/process_synchronization.c
${SRC_DIR}/reprompi_bench/sync/process_sync/window_sync.c
${SRC_DIR}/reprompi_bench/sync/common/sync_module_helpers.c
)


set(CACHING_SRC_FILES
${SRC_DIR}/reprompi_bench/caching/caching.c
${SRC_DIR}/reprompi_bench/caching/warm_cache.c
${SRC_DIR}/reprompi_bench/caching/clear_cache_memset.c
)


# generate commit version
find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify HEAD RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID )
    if( NOT ${res_var} EQUAL 0 )
        set(GIT_COM_ID "unknown")
        message(STATUS "Current directory is not a git repository. Build will not contain git revision info." )
    endif()
    string( REPLACE "\n" "" GIT_COMMIT_ID ${GIT_COM_ID} )
else()
    set(GIT_COMMIT_ID "unknown_git_not_found")
    message(STATUS "Git command not found. Build will not contain git revision info." )
endif()
configure_file("${SRC_DIR}/version.h.in" "${SRC_DIR}/version.h" @ONLY)


set(COMMON_OUTPUT_MAN_SRC_FILES
${SRC_DIR}/version.h
${SRC_DIR}/reprompi_bench/output_management/bench_info_output.c
${SRC_DIR}/reprompi_bench/output_management/runtimes_computation.c
)

set(CLOCKSYNC_HEADER_FILES
${SRC_DIR}/reprompi_bench/sync/clock_sync/clock_sync_common.h
${SRC_DIR}/reprompi_bench/sync/clock_sync/clock_sync_lib.h
${SRC_DIR}/reprompi_bench/sync/clock_sync/synchronization.h
)

set(CLOCKSYNC_HEADER_DIR ${SRC_DIR})

FetchContent_Declare(
        MPITS
        GIT_REPOSITORY https://github.com/hunsa/mpi-time-sync.git
        GIT_TAG cli_params)

FetchContent_MakeAvailable(MPITS)

export(EXPORT mpits-targets FILE "${PROJECT_BINARY_DIR}/mpits-targets.cmake")
set(MPITS_CONFIG ${CMAKE_INSTALL_PREFIX}/share/cmake/mpits/mpits-config.cmake)

if (NOT TARGET MPITS::mpits)
    message(FATAL "FAILED TO FIND MPITS")
endif()


set(REPROMPIBENCH_FILES
${SRC_DIR}/benchmarkMPIcalls.c
${SRC_DIR}/benchmarkCollective.c
${SRC_DIR}/benchmark_job.c
${SRC_DIR}/reprompi_bench/misc.c
${SRC_DIR}/reprompi_bench/utils/keyvalue_store.c
# synchronization methods
${SYNC_SRC_FILES}
# output
${COMMON_OUTPUT_MAN_SRC_FILES}
${SRC_DIR}/reprompi_bench/output_management/results_output.c
# command-line options
${COMMON_OPTION_PARSER_SRC_FILES}
${SRC_DIR}/reprompi_bench/option_parser/parse_timing_options.c
${SRC_DIR}/reprompi_bench/option_parser/parse_options.c
# collective functions
${COLL_OPS_SRC_FILES}
# caches cleaning strategy
${CACHING_SRC_FILES}
)

add_executable(mpibenchmark
${REPROMPIBENCH_FILES}
)
TARGET_LINK_LIBRARIES(mpibenchmark ${COMMON_LIBRARIES} MPITS::mpits MPI::MPI_C)
SET_TARGET_PROPERTIES(mpibenchmark PROPERTIES COMPILE_FLAGS "${MY_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(mpibenchmark PROPERTIES LINKER_LANGUAGE CXX )

# set the rpath for the executable
set_target_properties(mpibenchmark PROPERTIES
        BUILD_RPATH "$<TARGET_FILE_DIR:MPITS::mpits>"
        INSTALL_RPATH "$<TARGET_FILE_DIR:MPITS::mpits>"
        SKIP_BUILD_RPATH FALSE
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS mpibenchmark
        RUNTIME DESTINATION bin
)

if (ENABLE_BENCH_TESTS)
    ADD_SUBDIRECTORY(${SRC_DIR}/test)
endif()

if (ENABLE_SANITY_CHECK_CLOCK)
    ADD_SUBDIRECTORY(${SRC_DIR}/sanity_check/clock)
endif()

if(ENABLE_PGCHECKER)
    message(STATUS "Enabled: performance guidelines checker." )
    ADD_SUBDIRECTORY(${SRC_DIR}/pgcheck)
else()
    message(STATUS "Disabled: performance guidelines checker." )
endif()
