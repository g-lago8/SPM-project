

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
	std::vector<std::vector<float>> M(N, std::vector<float>(N, 0.0));

    //init

    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < N; ++j) {
            M[i][j] = 0;
        }
    }

    for (uint64_t i = 0; i < N; ++i) {
        M[i][i] = (float(i+1))/N;
    }

    // compute stencil
    auto start = std::chrono::steady_clock::now();
    compute_stencil_optim(M, N);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "Elapsed time (sequential): " << elapsed_seconds.count() << "s\n";
    
    std::ofstream file;
    file.open("../results/sequential.txt");
    file <<   elapsed_seconds.count() <<" " << N << std::endl;
    return 0;
}
