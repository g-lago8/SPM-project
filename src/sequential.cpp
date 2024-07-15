

#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <cassert>
#include <chrono>
#include<cmath>
#include<fstream>

void inline compute_stencil_one_pos(
    std::vector<std::vector<float>> &M,
    const uint64_t &N,
    const uint64_t &diag,
    const uint64_t &i)
{
    M[i][i+diag] = 0;
    for(uint64_t j =0; j<diag; ++j)
        M[i][i+diag] += M[i][i+j]*M[i+diag -j][i+diag];

    M[i][i+diag] = std::cbrt(M[i][i+diag]);
}

void compute_stencil(std::vector<std::vector<float>> &M, const uint64_t &N) {
    
    for(uint64_t diag = 1; diag< N; ++diag)        // for each upper diagonal
        for(uint64_t i = 0; i< (N-diag); ++i)      // for each elem. in the diagonal
            compute_stencil_one_pos(M, N, diag, i);  
    
}



int main( int argc, char *argv[] ) {
    uint64_t N = 512;    // default size of the matrix (NxN)
    
    if (argc != 1 && argc != 2) {
        std::printf("use: %s N\n", argv[0]);
        std::printf("     N size of the square matrix\n");
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
    compute_stencil(M, N);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "Elapsed time (sequential): " << elapsed_seconds.count() << "s\n";
    
    std::ofstream file;
    file.open("../results/sequential.txt");
    file <<   elapsed_seconds.count() <<" " << N << std::endl;
    return 0;
}
