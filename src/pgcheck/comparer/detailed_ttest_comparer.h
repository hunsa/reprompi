//
// Created by Max on 11/04/22.
//

#ifndef REPROMPI_DEV_SRC_PGCHECK_COMPARER_DETAILED_TTEST_COMPARER_H
#define REPROMPI_DEV_SRC_PGCHECK_COMPARER_DETAILED_TTEST_COMPARER_H

#include "../pgdata_comparer.h"

class DetailedTTestComparer : public PGDataComparer {

public:
  DetailedTTestComparer(std::string mpi_coll_name, int nnodes, int ppn);
  /**
   * @return data table in detailed t-test format
   */
  PGDataTable get_results();
};

#endif //REPROMPI_DEV_SRC_PGCHECK_COMPARER_DETAILED_TTEST_COMPARER_H
