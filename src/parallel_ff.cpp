#include "farm_wf.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>

void show_help(const char *program_name)
{
    std::printf("use: %s [N, nworkers, filename]\n", program_name);
    std::printf("Computes the wavefront of a matrix of size N with a FastFlow farm, using nworkers workers.\n"
                "This version divides work between the workers with a static block distribution. \n");

    std::printf("     N: size of the square matrix (default 2048)\n");
    std::printf("     nworkers: number of workers (default 4)\n");
    std::printf("     filename: name of the file to write the results to (default None, results are just printed to the console)\n");
}

void print_matrix(std::vector<std::vector<double>> &M)
{
    for (size_t i = 0; i < M.size(); i++)
    {
        for (size_t j = 0; j < M.size(); j++)
        {
            std::cout << std::fixed << std::setprecision(2) << M[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
int main(int argc, char *argv[])
{
    uint64_t N = 2048;    // default size of the matrix (NxN)
    int nworkers = 4;     // default number of workers
    size_t chunksize = 8; // default size of the chunk
    std::string filename = "strong_scaling_results2.txt";
    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))
    {
        show_help(argv[0]);
        return 0;
    }
    if (argc > 4)
    {
        std::printf("use: %s [N, nworkers]\n", argv[0]);
        std::printf("     N: size of the square matrix (default 2048)\n");
        std::printf("     nworkers: number of workers (default 4)\n");
        std::printf("     filename: name of the file to write the results to (default strong_scaling_results.txt)\n");
        return -1;
    }
    if (argc > 1)
    {
        N = std::stol(argv[1]);
    }
    if (argc > 2)
    {
        nworkers = std::stol(argv[2]);
    }
    if (argc > 3)
    {
        filename = argv[3];
    }

    if (N < 1)
    {
        std::cout << "Error: N must be greater than 0" << std::endl;
        return -1;
    }
    if (nworkers < 1)
    {
        std::cout << "Error: nworkers must be greater than 0" << std::endl;
        return -1;
    }
    if (chunksize < 1)
    {
        std::cout << "Error: chunksize must be greater than 0" << std::endl;
        return -1;
    }
    if (chunksize * nworkers > N)
    {
        chunksize = size_t(N / nworkers);

        std::cout << "Warning: chunksize * nworkers must be less than N, defaulting to N/nworkers = " << chunksize << std::endl;
    }

    std::vector<std::vector<double>> M(N, std::vector<double>(N, 0.0));

    for (uint64_t i = 0; i < N; ++i)
    {
        M[i][i] = double(i + 1) / double(N);
    }

    auto start = std::chrono::steady_clock::now();
    compute_stencil_par(M, N, nworkers);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
    // write time taken, number of workers, chunksize, and N to a file
    std::ofstream file;
    file.open(filename, std::ios_base::app);
    file << N << " " << nworkers << " " << elapsed_seconds.count() << std::endl;
    file.close();

    // print the matrix
    std::cout << M[0][N-1]<<std::endl;
    return 0;
}
