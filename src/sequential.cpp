

#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>
#include<cmath>
#include<fstream>
#include "sequential_wf.hpp"
#include <iomanip>

void print_matrix(std::vector<std::vector<double>> &M){
    for (size_t i = 0; i < M.size(); i++){
        for (size_t j = 0; j < M.size(); j++){
            std::cout << std::fixed << std::setprecision(2) << M[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

int main( int argc, char *argv[] ) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    auto filename = "result.txt";  // default name of the file to write the results to
    if (argc != 1 && argc != 2 && argc!=3) {
        std::printf("use: %s [N, filename]\n", argv[0]);
        std::printf("     N size of the square matrix (default 2048)\n");
        std::printf("     filename: name of the file to write the results to (default result.txt)\n");
        return -1;
    }
    if (argc > 1) {
        N = std::stol(argv[1]);
    }
    if (argc > 2) {
        filename = argv[2];
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

    // compute stencil
    auto start = std::chrono::steady_clock::now();
    compute_stencil_optim(M, N);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "Elapsed time (sequential): " << elapsed_seconds.count() << "s\n";
    
    // write the result to a file, append
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << N << " " << elapsed_seconds.count() << "\n";
        file.close();
    } else {
        std::cout << "Unable to open file\n";
    }
    std::cout <<M[0][N-1]<<std::endl;

    return 0;


}
