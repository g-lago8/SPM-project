#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <omp.h>

void inline compute_stencil_optim(std::vector<std::vector<double>> &M, const uint64_t &N) {
    for(uint64_t diag = 1; diag < N; ++diag) { // for each upper diagonal
        #pragma omp parallel for
        for(uint64_t i = 0; i < (N-diag); ++i) { // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            // #pragma omp parallel for reduction(+:temp)
            for (uint64_t j = 0; j < diag; ++j) { // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag][i_plus_diag - j];
            }
            M[i_plus_diag][i] = temp;
            M[i_plus_diag][i] = std::cbrt(M[i_plus_diag][i]); // cube root
            M[i][i_plus_diag] = M[i_plus_diag][i]; // store the result also in the upper triangle
        }
    }
}

void print_matrix(std::vector<std::vector<double>> &M) {
    for (size_t i = 0; i < M.size(); i++) {
        for (size_t j = 0; j < M.size(); j++) {
            std::cout << std::fixed << std::setprecision(2) << M[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char *argv[]) {
    uint64_t N = 2048; // default size of the matrix (NxN)

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

    // initialize the matrix
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < N; ++j) {
            M[i][j] = 0;
        }
    }

    for (uint64_t i = 0; i < N; ++i) {
        M[i][i] = (double(i+1)) / double(N);
    }

    // compute stencil
    auto start = std::chrono::steady_clock::now();
    compute_stencil_optim(M, N);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    std::cout << "Elapsed time (parallel): " << elapsed_seconds.count() << "s\n";

    // write the result to a file, append
    std::ofstream file("result.txt", std::ios::app);
    if (file.is_open()) {
        file << N << elapsed_seconds.count() << "\n";
        file.close();
    } else {
        std::cout << "Unable to open file\n";
    }
    if (N < 10) {
        print_matrix(M);
    }

    return 0;
}
