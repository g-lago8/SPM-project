#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>
#include<cmath>
#include<fstream>
#include "stencil_farm.hpp"



int main( int argc, char *argv[] ) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    
    if (argc != 1 && argc != 2) {
        std::printf("use: %s N\n", argv[0]);
        std::printf("     N size of the square matrix (default 2048)\n");
        return -1;
    }
    if (argc > 1) {
        N = std::stol(argv[1]);
    }


	// allocate the matrix
	std::vector<std::vector<double>> M(N, std::vector<double>(N, 0.0));

    //init

    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < N; ++j) {
            M[i][j] = 0;
        }
    }

    for (uint64_t i = 0; i < N; ++i) {
        M[i][i] = (double(i+1))/double(N);
    }
    auto M1 = M;
    auto M2 = M;
    auto M3 = M;
    // compute stencil naive
    auto start = std::chrono::steady_clock::now();
    compute_stencil_naive(M, N);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "Elapsed time (naive): " << elapsed_seconds.count() << "s\n";

    // compute stencil temp
    start = std::chrono::steady_clock::now();
    compute_stencil_temp(M1, N);
    end = std::chrono::steady_clock::now();
    elapsed_seconds = end-start;
    std::cout << "Elapsed time (temp): " << elapsed_seconds.count() << "s\n";

    // compute stencil with precomputing i_plus_diag
    start = std::chrono::steady_clock::now();
    compute_stencil_i_p_diag(M2, N);
    end = std::chrono::steady_clock::now();
    elapsed_seconds = end-start;
    std::cout << "Elapsed time (i_plus_diag): " << elapsed_seconds.count() << "s\n";

    // compute stencil optimized (save the result in the lower triangle)
    start = std::chrono::steady_clock::now();
    compute_stencil_optim(M3, N);
    end = std::chrono::steady_clock::now();
    elapsed_seconds = end-start;
    std::cout << "Elapsed time (all optimizations): " << elapsed_seconds.count() << "s\n";

    // compare the results
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = i; j < N; ++j) {
            if (abs(M[i][j]-M3[i][j]) > 1e-6) {
                std::cout << "Results differ at position (" << i << ", " << j << "): " << M[i][j] << " != " << M3[i][j] << std::endl;
                return -1;
            }
        }
    }
    std::cout << "All results are equal! Sburreck!\n";
}
