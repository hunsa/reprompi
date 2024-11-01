

#include <math.h>
#include <gsl/gsl_fit.h>

#include "HCAClockSync.h"
#include "LinearModelFitterStandard.hpp"

//#define ZF_LOG_LEVEL ZF_LOG_VERBOSE
#define ZF_LOG_LEVEL ZF_LOG_WARN
#include "log/zf_log.h"

HCAClockSync::HCAClockSync(ClockOffsetAlg *offsetAlg, int n_fitpoints) :
    HCAAbstractClockSync(offsetAlg, n_fitpoints) {
}

HCAClockSync::~HCAClockSync() {
}

/**
 * re-computes the offset (intercept) after learning one model after each round
 */
void HCAClockSync::remeasure_intercept_call_back(MPI_Comm comm, Clock &c, LinModel* lm, int client, int p_ref) {
  ZF_LOGV("compute intercept call back in HCA");
}

/**
 * re-computes the offset (intercept) after learning ALL models
 */
void HCAClockSync::remeasure_all_intercepts_call_back(MPI_Comm comm, Clock &c, LinModel* lm, const int ref_rank) {
  ZF_LOGV("compute all intercepts call back in HCA");

  // for HCA we always remeasure all intercepts at the end
  remeasure_all_intercepts(comm, c, lm, ref_rank);
}

GlobalClock* HCAClockSync::synchronize_all_clocks(MPI_Comm comm, Clock& c) {
  int my_rank, nprocs;
  int i, j, p;

  int master_rank = 0;
//  double *rtts_s;
//  int n_pingpongs = 1000;
  int max_power_two, nrounds_step1;
  //LinModel *linear_models, *tmp_linear_models;
  int running_power;
  int other_rank;
//  double current_rtt;
  int nb_lm_to_comm;
  LinModel lm;

  MPI_Datatype dtype[2] = { MPI_DOUBLE, MPI_DOUBLE };
  int blocklen[2] = { 1, 1 };
  MPI_Aint disp[2] = { 0, sizeof(double) };
  MPI_Datatype mpi_lm_t;
  MPI_Status stat;

  int *step_two_group_ranks;
  int step_two_nb_ranks;
  MPI_Group orig_group, step_two_group;
  MPI_Comm step_two_comm;

  MPI_Comm_rank(comm, &my_rank);
  MPI_Comm_size(comm, &nprocs);

  nrounds_step1 = floor(log2((double) nprocs));
  max_power_two = (int) pow(2.0, (double) nrounds_step1);

  LinModel linear_models[nprocs];
  LinModel tmp_linear_models[nprocs];

  MPI_Type_create_struct(2, blocklen, disp, dtype, &mpi_lm_t);
  MPI_Type_commit(&mpi_lm_t);

  for (i = 0; i < nrounds_step1; i++) {

    running_power = my_pow_2(i + 1);

    // get a client/server pair
    // : estimate rtt
    // : learn clock model
    // : gather clock model
    // : adjust model
    if (my_rank >= max_power_two) {
      //
    } else {
      if (my_rank % running_power == 0) {

        // master
        other_rank = my_rank + my_pow_2(i);

        // compute model through linear regression
        learn_model(comm, c, my_rank, other_rank);
        //compute_and_set_intercept(NULL, other_rank, my_rank);

        // so I also need to receive some other models from my partner rank
        // there should be 2^i models to receive
        nb_lm_to_comm = my_pow_2(i);

        if (nb_lm_to_comm > 0) {
          MPI_Recv(&tmp_linear_models[0], nb_lm_to_comm, mpi_lm_t, other_rank, 0, comm, &stat);

          linear_models[other_rank] = tmp_linear_models[0];
          for (j = 1; j < nb_lm_to_comm; j++) {
            linear_models[other_rank + j] = merge_linear_models(linear_models[other_rank], tmp_linear_models[j]);
          }
        }

      } else if (my_rank % running_power == my_pow_2(i)) {
        // client
        other_rank = my_rank - my_pow_2(i);

        // compute model through linear regression
        linear_models[my_rank] = learn_model(comm, c, other_rank, my_rank);
        //compute_and_set_intercept(&linear_models[my_rank], my_rank, other_rank);

        // I will need to send my models back to the master
        // there should be 2^i models to send
        // starting from my rank
        nb_lm_to_comm = my_pow_2(i);

        if (nb_lm_to_comm > 0) {
          MPI_Send(&linear_models[my_rank], nb_lm_to_comm, mpi_lm_t, other_rank, 0, comm);
        }
      }
    }
    MPI_Barrier(comm);
  }

  // now step 2
  // synchronize processes with ranks > 2^max_power_two
  if (nprocs > max_power_two) {
    MPI_Comm_group(comm, &orig_group);
    step_two_nb_ranks = nprocs - max_power_two + 1;
    step_two_group_ranks = new int[step_two_nb_ranks];
    step_two_group_ranks[0] = master_rank;
    j = 1;
    for (p = max_power_two; p < nprocs; p++) {
      step_two_group_ranks[j] = p;
      j++;
    }

    MPI_Group_incl(orig_group, step_two_nb_ranks, step_two_group_ranks, &step_two_group);

    MPI_Comm_create(comm, step_two_group, &step_two_comm);

    if (my_rank < max_power_two) {
      if (my_rank + max_power_two < nprocs) {
        other_rank = my_rank + max_power_two;

        // compute model through linear regression
        linear_models[other_rank] = learn_model(comm, c, my_rank, other_rank);
        //compute_and_set_intercept(NULL, other_rank, my_rank);
      }

    } else {
      other_rank = my_rank - max_power_two;

      // compute model through linear regression
      linear_models[my_rank] = learn_model(comm, c, other_rank, my_rank);
      //compute_and_set_intercept(&linear_models[my_rank], my_rank, other_rank);
    }

    if (step_two_comm != MPI_COMM_NULL) {
      // 0 in sub comm is master rank
      MPI_Gather(&linear_models[my_rank], 1, mpi_lm_t, &tmp_linear_models[0], 1, mpi_lm_t, 0, step_two_comm);
      MPI_Comm_free(&step_two_comm);
    }

    if (my_rank == master_rank) {
      // max_power_two ranks start in buffer at position 1
      for (j = 1; j < step_two_nb_ranks; j++) {
        // now we have the time between
        // p and p-max_power_two
        p = max_power_two - 1 + j;
        if (j == 1) { // master rank
          linear_models[p].slope = tmp_linear_models[j].slope;
          linear_models[p].intercept = tmp_linear_models[j].intercept;
        } else {
          linear_models[p] = merge_linear_models(linear_models[j - 1], tmp_linear_models[j]);
        }
      }
    }

    delete[] step_two_group_ranks;
  }

  MPI_Scatter(linear_models, 1, mpi_lm_t, &lm, 1, mpi_lm_t, master_rank, comm);
  remeasure_all_intercepts_call_back(comm, c, &lm, master_rank);

  MPI_Barrier(comm);

  if (my_rank == master_rank) {
    lm.slope = 0;
    lm.intercept = 0;
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &master_rank);
  MPI_Type_free(&mpi_lm_t);
  //printf("[rank %d] rank=%d s=%f i=%f nprocs=%d\n",master_rank, my_rank, lm.slope, lm.intercept,nprocs );
  return new GlobalClockLM(c, lm.slope, lm.intercept);
}
