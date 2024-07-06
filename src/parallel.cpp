#include<iostream>
#include<vector>
#include<chrono>
#include<random>
#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<ff/utils.hpp>

using namespace std;

struct Task{
    vector<vector<double>> &M;
    size_t N;
    size_t diag;
    size_t i;
    size_t chunksize;
};


void inline compute_stencil_one_chunk(
    std::vector<std::vector<double>> &M,
    const uint64_t &N,
    const uint64_t &diag,
    uint64_t &i,
    const uint64_t &chunksize)
{
    size_t end = min(i + chunksize, N-diag);
    for(; i<end; ++i){
        double temp = 0;
        for(uint64_t j =0; j<diag; ++j){
            temp += M[i][i+j]*M[i+diag -j][i+diag];
        }
        M[i][i+diag] = temp;

        M[i][i+diag] = std::cbrt(M[i][i+diag]);
    }
}


void inline compute_stencil(std::vector<std::vector<double>> &M, const uint64_t &N){
        for(uint64_t diag = 1; diag< N; ++diag)        // for each upper diagonal
            for(uint64_t i = 0; i< (N-diag); ++i){      // for each elem. in the diagonal
                auto temp = 0.0;
                for (uint64_t j = 0; j< diag; ++j)     // for each elem. in the stencil
                    temp += M[i][i+j]*M[i+diag -j][i+diag];
                M[i][i+diag] = temp;
                M[i][i+diag] = std::cbrt(M[i][i+diag]);
            }
}

struct Emitter: ff::ff_monode_t<bool, Task>{
    Emitter(vector<vector<double>> &M, size_t N, int n_workers,  size_t chunksize = 1):M(M), N(N), n_workers(n_workers), chunksize(chunksize) {}
    size_t diag =1;
    Task* svc(bool *diagonal_is_done){
        if(diagonal_is_done == nullptr){ // start of the stream

            // we define diagonal_is_done as a pointer to a boolean, and set it to true to start the computation
            diagonal_is_done = new bool;
            *diagonal_is_done = true;
        }
        // if the diagonal is not done, do nothing
        if (*diagonal_is_done == false) 
            return GO_ON;
        // else, send the tasks to the workers
        if( n_workers * chunksize > N - diag){
            chunksize = N / n_workers;
        }
        for(uint64_t i = 0; i< (N- diag); i += chunksize){      // for each elem. in the diagonal
            size_t block_size = min( N-diag-i, chunksize);
            *diagonal_is_done = false;
            ff_send_out(new Task{M, N, diag, i, block_size});
        }
        diag++;
        if (diag == N) return EOS;
        return GO_ON;
    }
    vector<vector<double>> &M;
    size_t N;
    int n_workers;
    size_t chunksize;
};


struct Worker: ff::ff_monode_t<Task, size_t>{
    size_t *svc(Task *task){
        compute_stencil_one_chunk(task->M, task->N, task->diag, task->i, task->chunksize);
        return &(task -> chunksize);
    }
};


struct Collector: ff::ff_minode_t<size_t, bool>{
    size_t done = 0;
    size_t N;
    size_t diag = 1;
    bool diagonal_is_done=false;
    Collector(size_t N):N(N){}
    bool *svc(size_t *computed){
        done += *computed;
        if (done == N-diag){
            done = 0;
            diag++;
            diagonal_is_done = true;
        }
        ff_send_out(&diagonal_is_done);
        return GO_ON;
    }
};


void compute_stencil_par(std::vector<std::vector<double>> &M, const uint64_t &N, int nworkers, size_t chunksize) {
    auto make_farm =[&]() {
                            vector<unique_ptr<ff::ff_node> > W;
                            for(auto i=0;i<nworkers;++i)
                                W.push_back(make_unique<Worker>());
                            return W;
                    };
    Emitter emitter(M, N, nworkers, chunksize);
    Collector collector(N);
    ff::ff_Farm<> farm(move(make_farm()), emitter, collector);
    farm.wrap_around();
    if (farm.run_and_wait_end()<0) {
        ff::error("running farm");
        return;
    }
}


int main( int argc, char *argv[] ) {
    uint64_t N = 1024;    // default size of the matrix (NxN)
    int nworkers = 4;    // default number of workers
    size_t chunksize =1; // default size of the chunk
    bool print_result = false;
    if (argc >5 || argc < 1) {
        std::printf("use: %s [N, nworkers, chunksize]\n", argv[0]);
        std::printf("     N: size of the square matrix\n");
        std::printf("     nworkers: number of workers\n");
        std::printf("     chunksize: size of the chunk\n");
        return -1;
    }
    if (argc > 1) {
        N = std::stol(argv[1]);
    }
    if (argc > 2) {
        nworkers = std::stol(argv[2]);
    }
    if (argc > 3) {
        chunksize = std::stol(argv[3]);
    }
    if (argc > 4) {
        print_result = bool(std::stol(argv[4]));
    }

    // check the input
    if (N < 1) {
        std::cout << "Error: N must be greater than 0" << std::endl;
        return -1;
    }
    if (nworkers < 1) {
        std::cout << "Error: nworkers must be greater than 0" << std::endl;
        return -1;
    }
    if (chunksize < 1) {
        std::cout << "Error: chunksize must be greater than 0" << std::endl;
        return -1;
    }
    if(chunksize * nworkers > N){
        std::cout << "Warning: chunksize * nworkers must be less than N, defaulting to N/nworkers" << std::endl;
        chunksize = size_t(N/nworkers);
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
        M[i][i] = double(i+1)/double(N);
    }
    auto M1 = M;

    // compute stencil parallel
    auto start = std::chrono::steady_clock::now();
    compute_stencil_par(M, N, nworkers, chunksize);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    if (print_result)
        std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

    // compute stencil sequential
    auto start_seq = std::chrono::steady_clock::now();
    compute_stencil(M1, N);
    auto end_seq = std::chrono::steady_clock::now();
    chrono::duration<double> elapsed_seconds_seq = end_seq-start_seq;

    if (print_result){
        cout << "elapsed time sequential: " << elapsed_seconds_seq.count() << "s\n";
        cout<<"speedup: "<<elapsed_seconds_seq/elapsed_seconds<<endl;
        cout<<"efficiency: "<<elapsed_seconds_seq/elapsed_seconds/nworkers<<endl;
    }

    // print M
    if(N<11){
        for (uint64_t i = 0; i < N; ++i) {
            for (uint64_t j = 0; j < N; ++j) {
                std::cout << M[i][j] << " ";
            }
            std::cout << std::endl;
        }
        for (uint64_t i = 0; i < N; ++i) {
            for (uint64_t j = 0; j < N; ++j) {
                std::cout << M1[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }

    // check the result
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < N; ++j) {
            if (abs(M[i][j] - M1[i][j]) > 1e-7){
                std::cout << "Error: M[" << i << "][" << j << "] = " << M[i][j] << " != " << M1[i][j] << std::endl;
                std::cout << "Difference: " <<abs(M[i][j] -M1[i][j]) << std::endl;
                return -1;
            }
        }
    }
    if (print_result)
        cout << "Sburreck!"<<endl;
    return 0;
}
