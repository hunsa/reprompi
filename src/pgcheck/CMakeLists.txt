
cmake_minimum_required(VERSION 3.22)

include(FetchContent)
include(ExternalProject)
include(GNUInstallDirs)

SET(PGTUNE_CMAKE_ARGS "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MACOSX_RPATH 1)

find_package(GSL REQUIRED)
if (GSL_INCLUDE_DIRS)
    message (STATUS "GSL INCLUDES : ${GSL_INCLUDE_DIRS}")
    message (STATUS "GSL LIBRARIES: ${GSL_LIBRARIES}")
else(GSL_INCLUDE_DIRS)
    message (FATAL_ERROR "GSL libraries not found.")
endif(GSL_INCLUDE_DIRS)


if(OPTION_ENABLE_LOGGING)
    SET(PGTUNE_CMAKE_ARGS "-DOPTION_ENABLE_LOGGING=ON ${PGTUNE_OPTIONS}")
    message(STATUS "Logging for pgtunelib enabled, cmake args: ${PGTUNE_CMAKE_ARGS}")
else()
    message(STATUS "Logging for pgtunelib disabled")
endif()

set(PGTUNELIB_PATH "" CACHE STRING "Path to pgmpicli library")
find_path(PGTUNELIB_INCLUDE_DIR pgmpi_tune.h PATHS ${PGTUNELIB_PATH}/include NO_DEFAULT_PATH)
find_library(PGTUNELIB_LIB NAMES pgmpicli PATHS ${PGTUNELIB_PATH}/lib ${PGTUNELIB_PATH}/lib64 NO_DEFAULT_PATH)

message(STATUS "PGTUNELIB_INCLUDE_DIR: ${PGTUNELIB_INCLUDE_DIR}")

if(NOT PGTUNELIB_INCLUDE_DIR)
	message(FATAL_ERROR "pgmpi_tune.h not found. Please set PGTUNELIB_PATH to enable this library.")
else()
    message(STATUS "pgmpi_tune.h found in ${PGTUNELIB_PATH}/include")
endif()

if(NOT PGTUNELIB_LIB)
    message(FATAL_ERROR "pgmpicl not found. Please set PGTUNELIB_PATH to enable this library.")
else()
    message(STATUS "pgmpicli found: ${PGTUNELIB_LIB}.")
    get_filename_component(PGMPICLI_LIB_DIR ${PGTUNELIB_LIB} DIRECTORY)
endif()

set(PGTUNE_CONFIG_CSV ${CMAKE_INSTALL_PREFIX}/pgmpi_conf.csv)
add_custom_target(config
        COMMAND  bash -c "exec ${PGTUNELIB_PATH}/bin/pgmpi_info > ${PGTUNE_CONFIG_CSV}"
        VERBATIM
)


list(APPEND CMAKE_BUILD_RPATH "${PGMPICLI_LIB_DIR}")

add_executable(pgchecker
        pgchecker.cpp
        utils/csv_parser.cpp
        pgcheck_helper.cpp
        pgcheck_options.cpp
        pgtunelib_interface.cpp
        pgdata.cpp
        pgdata_table.cpp
        pgdata_comparer.cpp
        pgdata_printer.cpp
        comparer/statistical_test/ttest.cpp
        comparer/statistical_test/wilcoxon_mann_whitney.cpp
        comparer/statistical_test/two_sample_test_factory.cpp
        comparer/statistical_test/two_sample_test.cpp
        comparer/comparer_data.cpp
        comparer/comparer_factory.cpp
        comparer/simple_comparer.cpp
        comparer/violation/detailed_violation_comparer.cpp
        comparer/violation/violation_comparer.cpp
        comparer/violation/grouped_violation_comparer.cpp
        comparer/runtime/abs_runtime_comparer.cpp
        comparer/runtime/rel_runtime_comparer.cpp
        comparer/raw/raw_comparer.cpp
        logger/logger.cpp
        pgcheck_input.cpp
        utils/string_utils.cpp
        utils/time_utils.cpp
        utils/argv_manager.cpp

        ${SRC_DIR}/benchmark_job.c
        ${SRC_DIR}/benchmarkCollective.c
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
        #${SRC_DIR}/reprompi_bench/option_parser/parse_extra_key_value_options.c
        ${SRC_DIR}/reprompi_bench/option_parser/parse_options.c
        # collective functions
        ${COLL_OPS_SRC_FILES}
        # caches cleaning strategy
        ${CACHING_SRC_FILES}
)


target_include_directories(pgchecker PRIVATE
        ${GSL_INCLUDE_DIRS}
        ${PGTUNELIB_INCLUDE_DIR})
target_link_directories(pgchecker PRIVATE "${PGMPICLI_LIB_DIR}")

TARGET_LINK_LIBRARIES(pgchecker ${GSL_LIBRARIES} ${PGTUNELIB_LIB} MPITS::mpits MPI::MPI_CXX)

# set the rpath for the executable
# but take care of mpits
set_target_properties(pgchecker PROPERTIES
        INSTALL_RPATH "${PGMPICLI_LIB_DIR};$<TARGET_FILE_DIR:MPITS::mpits>"
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS pgchecker
        RUNTIME DESTINATION bin
)