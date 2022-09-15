#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>
//@ add_includes


char* to_string(int n) {
    char *s;
    int SIZE=30;
    s = (char*)malloc(SIZE * sizeof(char));
    sprintf(s, "%d", n);
    return s;
}

int main(int argc, char *argv[])
{
    int i, j, count, k;
    int n_procs = 0;
    void *send_buffer, *recv_buffer;
    int rank;
    char* meas_functions[] = {"MPI_Alltoall", "MPI_Bcast"};
    int n_calls = 2;
    int max_count = 50;
    int inc_count=10;
    int call_rep = 4;
    //@ declare_variables


    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size( MPI_COMM_WORLD, &n_procs);


    //@ initialize_bench
    //@ initialize_timestamps t1
    //@ initialize_timestamps t2

    //@ global datatype="MPI_BYTE"
    //@ global program_name="triple_loop_test"
    for (i=0; i<n_calls; i++) {
        for (j=1; j< call_rep; j++ ) {
            for (count=0; count<max_count; count+=inc_count) {

                //@ set callname=meas_functions[i]

                send_buffer = malloc( n_procs * count);
                recv_buffer = malloc( n_procs * count);

                //@ start_measurement_loop

                //@ measure_timestamp t1
                if (strcmp(meas_functions[i], "MPI_Bcast") == 0) {
                    MPI_Bcast(send_buffer, count, MPI_BYTE, 0, MPI_COMM_WORLD);
                }
                else {
                    MPI_Alltoall( send_buffer, count, MPI_BYTE, recv_buffer,
                            count, MPI_BYTE, MPI_COMM_WORLD);
                }
                //@ measure_timestamp t2

                //@ stop_measurement_loop

                //@ print_runtime_array name=runtime_coll start_time=t1 end_time=t2 type=reduce op=max testname=callname count=count ncalls=j

                free( send_buffer);
                send_buffer = NULL;
                free( recv_buffer);
                recv_buffer = NULL;
            }
        }
    }

    //@cleanup_bench

    MPI_Finalize();

    return 0;
}
