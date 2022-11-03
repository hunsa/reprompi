
#ifndef REPROMPI_SRC_PGCHECK_PGDATA_COMPARER_H
#define REPROMPI_SRC_PGCHECK_PGDATA_COMPARER_H

#include "pgdata.h"
#include "comparer/comparer_data.h"
#include "utils/statistics_utils.h"
#include "utils/statistics_utils.cpp"
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <numeric>

class PGDataComparer {

protected:
  std::string mpi_coll_name;
  int nnodes;  // number of nodes
  int ppn;     // number of processes per node
  std::unordered_map<std::string, PGData *> mockup2data;
  double barrier_time_s = -1.0;

public:

  PGDataComparer(std::string mpi_coll_name, int nnodes, int ppn);
  virtual ~PGDataComparer() {};

  void add_dataframe(std::string mockup_name, PGData *data);

  virtual std::string get_results() = 0;

  /**
   * adds the mean time for MPI_Barrier in seconds
   */
  void add_barrier_time(double time_s);

  /**
   *
   * @return saved MPI_Barrier time in seconds
   */
  double get_barrier_time();

  /**
   *
   * @return true if barrier time has been set before
   */
  bool has_barrier_time();

};

#endif //REPROMPI_SRC_PGCHECK_PGDATA_COMPARER_H
