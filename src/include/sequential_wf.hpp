#ifndef SEQUENTIAL_WF_HPP
#define SEQUENTIAL_WF_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>

void inline compute_stencil_naive(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // naive implementation of the stencil computation, none of the optimizations are applied
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                M[i][i+diag]  += M[i][i+j] * M[i+diag-j][i+diag];
            }

            M[i][i+diag] = std::cbrt(M[i][i+diag]); // cube root
        }
    }
}

void inline compute_stencil_temp(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here, we accumulate the result in a temporary variable, to avoid writing to the same memory location multiple times.
    // In a sequential program, this would not change much, but in a parallel program, it can be faster, avoiding false sharing.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag-j][i_plus_diag];
            }
            M[i][i_plus_diag] = temp;
            M[i][i_plus_diag] = std::cbrt(M[i][i_plus_diag]); // cube root
        }
    }
}
void inline compute_stencil_i_p_diag(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here we compute one time i_plus_diag =i+diag, that is used multiple times in the inner loop
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag-j][i_plus_diag];
            }
            M[i][i_plus_diag] = temp;
            M[i][i_plus_diag] = std::cbrt(M[i][i_plus_diag]); // cube root
        }
    }
}

void inline compute_stencil_optim(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here we compute the stencil in a more cache-friendly way, by storing the result in the lower triangle, and copying it to the upper triangle, 
    // in order to do a dot product over two rows, instead of a dot product between a row and a column.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag][i_plus_diag -j];
            }
            M[i_plus_diag][i] =temp;
            M[i_plus_diag][i] = std::cbrt(M[i_plus_diag][i]); // cube root
            M[i][i_plus_diag] = M[i_plus_diag][i]; // store the result also in the upper triangle

        }
    }
}

#endif