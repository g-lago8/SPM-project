#include "stencil_farm.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers
    size_t chunksize = 8; // default size of the chunk
    bool on_demand =false;
    std::string filename = "strong_scaling_results.txt";
    
    if(argc > 6) {
        std::printf("use: %s [N, nworkers, chunksize]\n", argv[0]);
        std::printf("     N: size of the square matrix (default 2048)\n");
        std::printf("     nworkers: number of workers (default 4)\n");
        std::printf("     chunksize: size of the chunk (default 8)\n");
        std::printf("     on_demand: whether or not to set on-demand scheduling (default false, round robin)\n");
        return -1;
    }
    if(argc > 1) {
        N = std::stol(argv[1]);
    }
    if(argc > 2) {
        nworkers = std::stol(argv[2]);
    }
    if(argc > 3) {
        chunksize = std::stol(argv[3]);
    }
    if(argc >4){
        on_demand = bool(std::stol(argv[4]));
    }
    if(argc >5){
        filename = argv[5];
    }

    if(N < 1) {
        std::cout << "Error: N must be greater than 0" << std::endl;
        return -1;
    }
    if(nworkers < 1) {
        std::cout << "Error: nworkers must be greater than 0" << std::endl;
        return -1;
    }
    if(chunksize < 1) {
        std::cout << "Error: chunksize must be greater than 0" << std::endl;
        return -1;
    }
    if(chunksize * nworkers > N) {
        std::cout << "Warning: chunksize * nworkers must be less than N, defaulting to N/nworkers" << std::endl;
        chunksize = size_t(N/nworkers);
    }



    // Initialization of M as a flat vector
    std::vector<double> M(N * N, 0.0);

    // Accessing and initializing elements in M
    for(uint64_t i = 0; i < N; ++i) {
        for(uint64_t j = 0; j < N; ++j) {
            M[i*N + j] = 0; // This line is actually redundant given the initialization above
        }
    }
    // Setting diagonal elements
    for(uint64_t i = 0; i < N; ++i) {
        M[i*N + i] = double(i+1)/double(N);
    }

    // parallel
    compute_stencil_par(M, N, nworkers, chunksize, on_demand);
    // sequential
    auto M1 = M;
    compute_stencil(M1, N);

    // check correctness
    for(uint64_t i = 0; i < N; ++i) {
        for(uint64_t j = 0; j < N; ++j) {
            if (abs(M[i*N + j] - M1[i*N + j]) > 1e-6) {
                std::cout << "Correctness check failed!" << std::endl;
                return -1;
            }
        }
    }
    std::cout << "Correctness check passed!" << std::endl;
}


