#include "stencil_farm.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers
    size_t chunksize = 8; // default size of the chunk
    bool on_demand =false;
    
    if(argc > 5) {
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

    auto start = std::chrono::steady_clock::now();
    compute_stencil_par(M, N, nworkers, chunksize, on_demand);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
    // write time taken, number of workers, chunksize, and N to a file
    std::ofstream file;
    file.open("../results/strong_scaling_results.txt", std::ios_base::app);
    file << elapsed_seconds.count() << " " << nworkers << " " << chunksize << " " << N << std::endl;
    file.close();

    return 0;
}


