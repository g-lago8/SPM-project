#include "farm_wf.hpp"
#include "sequential_wf.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>


void print_matrix(std::vector<std::vector<double>> &M){
    for (size_t i = 0; i < M.size(); i++){
        for (size_t j = 0; j < M.size(); j++){
            std::cout << std::fixed << std::setprecision(2) << M[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
int main(int argc, char *argv[]) {
    uint64_t N = 2048;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers
    std::string filename ;
    if(argc > 4) {
        std::printf("use: %s [N, nworkers]\n", argv[0]);
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
    if(argc >3){
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
    // compute the value ofN scaled by the cube root of the number of workers
    long double N_ld = N;
    long double nworkers_ld = nworkers;
    N = N_ld * std::cbrt(nworkers_ld);
    size_t N_sz = N;
    std::cout << "N: " << N_sz << std::endl;

    std::vector<std::vector<double>> M(N_sz, std::vector<double>(N_sz, 0.0));

    for(uint64_t i = 0; i < N_sz; ++i) {
        M[i][i] = double(i+1)/double(N);
    }

    auto start = std::chrono::steady_clock::now();
    compute_stencil_par(M, N_sz, nworkers);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    start = std::chrono::steady_clock::now();
    compute_stencil_optim(M, N_sz);
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds_seq = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
    std::cout << "elapsed time sequential: " << elapsed_seconds_seq.count() << "s\n";
    // write time taken, number of workers, chunksize, and N to a file

    if (argc > 3) {
        std::cout << "writing to file" << filename << std::endl;
        std::ofstream file(filename, std::ios::app);

        if (file.is_open()) {
            file << N_sz << " " << nworkers     << " " << elapsed_seconds.count() << "\n";
            file << N_sz << " " << "sequential" << " " << elapsed_seconds_seq.count() << " " << "\n";
            file.close();
        } else {
            std::cout << "Unable to open file\n";
        }
    }
    return 0;
}


