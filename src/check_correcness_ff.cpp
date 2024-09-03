#include "farm_wf.hpp"
#include "sequential_wf.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers

    std::string filename = "strong_scaling_results.txt";
    
    if(argc > 4) {
        std::printf("use: %s [N, nworkers, chunksize]\n", argv[0]);
        std::printf("     N: size of the square matrix (default 2048)\n");
        std::printf("     nworkers: number of workers (default 4)\n");
        std::printf("     filename: name of the file to write the results to (default strong_scaling_results.txt)\n");
        return -1;
    }
    if(argc > 1) {
        N = std::stol(argv[1]);
    }
    if(argc > 2) {
        nworkers = std::stol(argv[2]);
    }
    if(argc > 3){
        filename = argv[3];
    }

    if(N < 1) {
        std::cout << "Error: N must be greater than 0" << std::endl;
        return -1;
    }
    if(nworkers < 1) {
        std::cout << "Error: nworkers must be greater than 0" << std::endl;
        return -1;
    }

    std::vector<std::vector<double>> M(N, std::vector<double>(N, 0.0));

    for(uint64_t i = 0; i < N; ++i) {
        for(uint64_t j = 0; j < N; ++j) {
            M[i][j] = 0;
        }
    }
    for(uint64_t i = 0; i < N; ++i) {
        M[i][i] = double(i+1)/double(N);
    }

    auto M1 = M;
    compute_stencil_par(M, N, nworkers);
    compute_stencil_optim(M1, N);

    // check correctness
    for(uint64_t i = 0; i < N; ++i) {
        for(uint64_t j = i; j < N; ++j) {
            if (abs(M[i][j] - M1[i][j]) > 1e-6) {
                std::cout << "Error: M[" << i << "][" << j << "] = " << M[i][j] << " != " << M1[i][j] << std::endl;
                return -1;
            }
        }
    }

    std::cout << "Correctness check passed" << std::endl;

    return 0;
}


