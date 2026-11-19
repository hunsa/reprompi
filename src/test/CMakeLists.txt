add_executable(test_dict
test_global_dict.c
${SRC_DIR}/reprompi_bench/misc.c
${SRC_DIR}/reprompi_bench/utils/keyvalue_store.c
)
TARGET_LINK_LIBRARIES(test_dict ${COMMON_LIBRARIES} MPI::MPI_C)

