
/* Copyright (c) 2019 Bradley Worley <geekysuavo@gmail.com>
 * Released under the MIT License.
 *
 * Compilation:
 *   g++ -std=c++14 -O3 ista.cc -o ista -lfftw3 -lm
 */

#include "inst.hh"

int main (int argc, char **argv) {
  /* read the instance data from disk. */
  auto data = load(argc, argv);

  /* problem sizes. */
  const std::size_t m = data.size();
  const std::size_t n = 2048;

  /* prepare fftw. */
  fft<n> F;
  auto dy = F.data();

  /* number of iterations. */
  const std::size_t iters = 1000;

  /* threshold reduction factor. */
  const double mu = 0.98;

  /* construct the schedule and measured
   * vectors from the data table.
   */
  auto S = schedule_vector(data, n);
  auto y = measured_vector(data, n);

  /* allocate x, fx. */
  complex_vector x{new double[n][K]};
  complex_vector fx{new double[n][K]};
  for (std::size_t i = 0; i < n; i++)
    for (std::size_t k = 0; k < K; k++)
      x[i][k] = fx[i][k] = 0;

  /* compute the initial thresholding value. */
  for (std::size_t i = 0; i < n; i++)
    for (std::size_t k = 0; k < K; k++)
      dy[i][k] = y[i][k];
  F.fwd();
  double thresh = 0;
  for (std::size_t i = 0; i < n; i++) {
    double dy2 = 0;
    for (std::size_t k = 0; k < K; k++) {
      dy[i][k] /= std::sqrt(n);
      dy2 += std::pow(dy[i][k], 2);
    }

    thresh = std::max(thresh, mu * std::sqrt(dy2));
  }

  /* iterate. */
  for (std::size_t it = 0; it < iters; it++) {
    /* compute the current spectral estimate. */
    for (std::size_t i = 0; i < n; i++)
      for (std::size_t k = 0; k < K; k++)
        dy[i][k] = S[i] * (y[i][k] - x[i][k]);
    F.fwd();
    for (std::size_t i = 0; i < n; i++)
      for (std::size_t k = 0; k < K; k++)
        fx[i][k] += dy[i][k] / std::sqrt(n);

    /* apply the l1 function. */
    for (std::size_t i = 0; i < n; i++) {
      double fxnrm = 0;
      for (std::size_t k = 0; k < K; k++)
        fxnrm += std::pow(fx[i][k], 2);

      fxnrm = std::sqrt(fxnrm);
      for (std::size_t k = 0; k < K; k++)
        fx[i][k] *= (fxnrm > thresh ? 1 - thresh / fxnrm : 0);
    }

    /* update the time-domain estimate. */
    for (std::size_t i = 0; i < n; i++)
      for (std::size_t k = 0; k < K; k++)
        dy[i][k] = fx[i][k];
    F.inv();
    for (std::size_t i = 0; i < n; i++)
      for (std::size_t k = 0; k < K; k++)
        x[i][k] = dy[i][k] / std::sqrt(n);

    /* update the threshold. */
    thresh *= mu;
  }

  /* output the results. */
  std::cout.precision(9);
  std::cout << std::scientific;
  for (std::size_t i = 0; i < n; i++)
    std::cout << i << " "
              << fx[i][0] << " "
              << fx[i][1] << "\n";
}

