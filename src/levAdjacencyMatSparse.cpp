#include <RcppArmadillo.h>
#include <strings.h>
#include "levDistBounded.h"
using namespace arma;

// [[Rcpp::export]]
arma::sp_umat levAdjacencyMatSparse(std::vector<std::string> strings,
                                    const int& maxdist) {

  // allocate memory for data structures
  const int dim = strings.size();
  int dist;
  arma::sp_umat out = speye<sp_umat>(dim, dim);

  // compute adjacencies for upper triangle
  for (int j = 0; j < dim; ++j) {           // columns
    for (int i = 0; i < j; ++i) {           // rows
      dist = levDistBounded(strings[i], strings[j], maxdist);
      if (dist != -1) { out(i, j) = 1; }
    }
  }

  // reflect upper triangle to lower
  out = arma::symmatu(out);

  // sum entries columnwise
  arma::sp_umat col_sums_spmat = arma::sum(out);
  arma::urowvec col_sums(col_sums_spmat);

  // record indices of nodes with positive degree
  arma::uvec col_ids = find(col_sums > 1);

  // write indices of network nodes to file
  col_ids.save("col_ids.txt", raw_ascii);

  // subset matrix to keep only network nodes
  out = out.cols(col_ids);
  out = out.t();
  out = out.cols(col_ids);

  return(out);

}
