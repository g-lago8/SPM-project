#include "stencil_farm.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
    uint64_t N = 512;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers
    size_t chunksize = 1; // default size of the chunk
    
    if(argc != 1 && argc != 2 && argc != 3 && argc != 4) {
        std::printf("use: %s [N, nworkers, chunksize]\n", argv[0]);
        std::printf("     N: size of the square matrix\n");
        std::printf("     nworkers: number of workers\n");
        std::printf("     chunksize: size of the chunk\n");
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

    auto start = std::chrono::steady_clock::now();
    compute_stencil_par(M, N, nworkers, chunksize);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

    auto start_seq = std::chrono::steady_clock::now();
    compute_stencil(M1, N);
    auto end_seq = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds_seq = end_seq-start_seq;
    std::cout << "elapsed time sequential: " << elapsed_seconds_seq.count() << "s\n";
    std::cout << "speedup: " << elapsed_seconds_seq / elapsed_seconds << std::endl;
    std::cout << "efficiency: " << elapsed_seconds_seq / elapsed_seconds / nworkers << std::endl;

    if(N < 11) {
        for(uint64_t i = 0; i < N; ++i) {
            for(uint64_t j = 0; j < N; ++j) {
                std::cout << M[i][j] << " ";
            }
            std::cout << std::endl;
        }
        for(uint64_t i = 0; i < N; ++i) {
            for(uint64_t j = 0; j < N; ++j) {
                std::cout << M1[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }

    for(uint64_t i = 0; i < N; ++i) {
        for(uint64_t j = 0; j < N; ++j) {
            if(std::abs(M[i][j] - M1[i][j]) > 1e-7) {
                std::cout << "Error: M[" << i << "][" << j << "] = " << M[i][j] << " != " << M1[i][j] << std::endl;
                std::cout << "Difference: " << std::abs(M[i][j] - M1[i][j]) << std::endl;
                return -1;
            }
        }
    }
    std::cout << "Sburreck!" << std::endl;
    return 0;
}


