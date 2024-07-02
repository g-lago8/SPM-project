#include<iostream>
#include<vector>
#include<chrono>
#include<random>

#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include<ff/utils.hpp>
using namespace std;

struct Task{
    vector<vector<float>> &M;
    size_t N;
    size_t diag;
    size_t i;
};

void compute_stencil_one_pos(
    std::vector<std::vector<float>> &M,
    const uint64_t &N,
    const uint64_t &diag,
    const uint64_t &i)
{
    M[i][i+diag] = 0;
    for(uint64_t j =0; j<diag; ++j)
        M[i][i+diag] += M[i][i+j]*M[i+diag -j][i+diag];

    //M[i][i+diag] = std::cbrt(M[i][i+diag]);
}

void compute_stencil(std::vector<std::vector<float>> &M, const uint64_t &N){
        
        for(uint64_t diag = 1; diag< N; ++diag)        // for each upper diagonal
            for(uint64_t i = 0; i< (N-diag); ++i)      // for each elem. in the diagonal
                compute_stencil_one_pos(M, N, diag, i);  
        
    
}

struct DiagonalSelector: ff::ff_minode_t<float, size_t>{
    size_t N;
    size_t count_done = N-1;
    DiagonalSelector(size_t N): N(N){}
    size_t diag = 0;

    size_t* svc(float *task){

        if(++count_done == N-diag){
            diag++;
            count_done = 0;
            return &diag;
        }
        
        if (diag == N-1) return EOS;
        return GO_ON;

    }
};

struct ElementDispatcher: ff::ff_monode_t<size_t, Task>{
    ElementDispatcher(vector<vector<float>> &M, size_t N):M(M), N(N){}
    Task* svc(size_t *diag){
        for(uint64_t i = 0; i< (N- *diag); ++i){      // for each elem. in the diagonal
            ff_send_out(new Task{M, N, *diag, i});
        }
        // wait for the workers to finish
        if (*diag == N-1) return EOS;
        return GO_ON;
    }
    vector<vector<float>> &M;
    size_t N;
};

struct Worker: ff::ff_monode_t<Task, float>{
    float *svc(Task *task){
        float *foo = new float;
        compute_stencil_one_pos(task->M, task->N, task->diag, task->i);
        ff_send_out_to(foo, 0);
        return GO_ON;
    }
};







void compute_stencil(std::vector<std::vector<float>> &M, const uint64_t &N, int nworkers) {

    auto make_farm =[&]() {
                            std::vector<std::unique_ptr<ff::ff_node> > W;
                            for(auto i=0;i<nworkers;++i)
                                W.push_back(make_unique<Worker>());
                            return W;
                    };

    // make the farm
    ff::ff_Farm<Task> farm(std::move(make_farm()));
    farm.remove_collector();
    DiagonalSelector ds(N);
    ElementDispatcher ed(M, N);
    ff::ff_Pipe<Task> pipe(ds, ed, farm);
    pipe.wrap_around();
    // run the pipe
    if (pipe.run_and_wait_end()<0) {
        ff::error("running pipe");
    }
    
}

int main( int argc, char *argv[] ) {
    uint64_t N = 512;    // default size of the matrix (NxN)
    int nworkers = 4;
    
    if (argc != 1 && argc != 2 && argc != 3) {
        std::printf("use: %s N [nworkers]\n", argv[0]);
        std::printf("     N size of the square matrix\n");
        return -1;
    }
    if (argc > 1) {
        N = std::stol(argv[1]);
    }
    if (argc > 2) {
        nworkers = std::stol(argv[2]);
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
        M[i][i] = 0.1;
    }
    auto M1 = M;
    // compute stencil
    auto start = std::chrono::steady_clock::now();
    compute_stencil(M, N, nworkers);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";

    auto start_seq = std::chrono::steady_clock::now();
    compute_stencil(M1, N);
    auto end_seq = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds_seq = end_seq-start_seq;
    std::cout << "elapsed time sequential: " << elapsed_seconds_seq.count() << "s\n";

    // print M
    if(N<11){
        for (uint64_t i = 0; i < N; ++i) {
            for (uint64_t j = 0; j < N; ++j) {
                std::cout << M[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
    // check the result
    for (uint64_t i = 0; i < N; ++i) {
        for (uint64_t j = 0; j < N; ++j) {
            if (M[i][j] != M1[i][j]) {
                std::cout << "Error: M[" << i << "][" << j << "] = " << M[i][j] << " != " << M1[i][j] << std::endl;
                return -1;
            }
        }
    }
    return 0;
}
