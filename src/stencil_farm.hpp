#ifndef STENCIL_HPP
#define STENCIL_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <ff/utils.hpp>


void inline compute_stencil_naive(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // naive implementation of the stencil computation, none of the optimizations are applied
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                M[i][i+diag]  += M[i][i+j] * M[i+diag-j][i+diag];
            }

            M[i][i+diag] = std::cbrt(M[i][i+diag]); // cube root
        }
    }
}

void inline compute_stencil_temp(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here, we accumulate the result in a temporary variable, to avoid writing to the same memory location multiple times.
    // In a sequential program, this would not change much, but in a parallel program, it can be faster, avoiding false sharing.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag-j][i_plus_diag];
            }
            M[i][i_plus_diag] = temp;
            M[i][i_plus_diag] = std::cbrt(M[i][i_plus_diag]); // cube root
        }
    }
}
void inline compute_stencil_i_p_diag(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here we compute one time i_plus_diag =i+diag, that is used multiple times in the inner loop
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag-j][i_plus_diag];
            }
            M[i][i_plus_diag] = temp;
            M[i][i_plus_diag] = std::cbrt(M[i][i_plus_diag]); // cube root
        }
    }
}

void inline compute_stencil_optim(std::vector<std::vector<double>> &M, const uint64_t &N) {
    // here we compute the stencil in a more cache-friendly way, by storing the result in the lower triangle, and copying it to the upper triangle, 
    // in order to do a dot product over two rows, instead of a dot product between a row and a column.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i][i+j] * M[i_plus_diag][i_plus_diag -j];
            }
            M[i_plus_diag][i] =temp;
            M[i_plus_diag][i] = std::cbrt(M[i_plus_diag][i]); // cube root
            M[i][i_plus_diag] = M[i_plus_diag][i]; // store the result also in the upper triangle

        }
    }
}
// ------------------------------------------------------------------
// ---------------------- FARM IMPLEMENTATION -----------------------
// ------------------------------------------------------------------
struct Task {
    std::vector <std::vector<double>> &M;
    size_t N;
    size_t diag;
    size_t i;
    size_t chunksize;
};


void inline compute_stencil_one_chunk(
    // Function used by the workers in the farm, computes a piece of the wavefront.
    std::vector<std::vector<double>> &M,
    const uint64_t &N,
    const uint64_t &diag,
    uint64_t &i,
    const uint64_t &chunksize) 
{
    size_t end = std::min(i + chunksize, N-diag);
    for(; i < end; ++i) {
        double temp = 0;
        auto i_plus_diag = i + diag; 
        for(uint64_t j = 0; j < diag; ++j) {
            temp += M[i][i+j] * M[i_plus_diag][i_plus_diag - j];
        }

        // we store the result in the lower triangle, to do a dot product over two rows, 
        //instead of a dot product between a row and a column (better cache locality)
        M[i_plus_diag][i] =temp; 
        M[i_plus_diag][i] = std::cbrt(M[i_plus_diag][i]); // cube root
        M[i][i_plus_diag] = M[i_plus_diag][i]; // store the result also in the upper triangle

    }
}

struct Emitter: ff::ff_monode_t<bool, Task>{
    Emitter(std::vector<std::vector<double>> &M, size_t N, int n_workers,  size_t chunksize = 1):M(M), N(N), n_workers(n_workers), chunksize(chunksize) {
        total_time = 0.0;
    }
    size_t diag =1;
    double total_time;

    Task* svc(bool *diagonal_is_done){
        auto start = std::chrono::high_resolution_clock::now();
        // send the tasks to the workers
        if( n_workers * chunksize > N - diag){
            chunksize = N / n_workers;
        }

        if(diagonal_is_done!=nullptr){
            *diagonal_is_done = false;
        }
        
        for(uint64_t row = 0; row< (N- diag); row += chunksize){      // for each elem. in the diagonal
            size_t block_size = std::min( N-diag-row, chunksize);
            ff_send_out(new Task{M, N, diag, row, block_size});
        }
        diag++;
        auto end = std::chrono::high_resolution_clock::now();
        total_time += std::chrono::duration<double>(end-start).count();
        if (diag == N) return EOS;
        return GO_ON;
    }


    std::vector<std::vector<double>> &M;
    size_t N;
    int n_workers;
    size_t chunksize;
};

struct Worker: ff::ff_monode_t<Task, Task> {
    double total_time=0.0;
    Task* svc(Task *task) {
        auto start = std::chrono::high_resolution_clock::now();
        compute_stencil_one_chunk(task->M, task->N, task->diag, task->i, task->chunksize);
        auto end = std::chrono::high_resolution_clock::now();
        total_time += std::chrono::duration<double>(end-start).count();
        return task;
    }
};

struct Collector: ff::ff_minode_t<Task, bool> {
    double total_time;

    Collector(size_t N): N(N) {total_time = 0.0;}

    bool* svc(Task *computed) {
        auto start = std::chrono::high_resolution_clock::now();
        done += computed->chunksize;
        delete computed;
        if(done == N-diag) { // if the diagonal is all done
            done = 0;
            diag++;
            diagonal_is_done = true;
            auto end = std::chrono::high_resolution_clock::now();
            total_time += std::chrono::duration<double>(end-start).count();
            return &diagonal_is_done; // send the signal to the emitter

        }
        auto end = std::chrono::high_resolution_clock::now();
        total_time += std::chrono::duration<double>(end-start).count();
        return GO_ON; // else do nothing and keep going
    }


    size_t done = 0;
    size_t N;
    size_t diag = 1;
    bool diagonal_is_done = false;
};


void compute_stencil_par(std::vector<std::vector<double>> &M, const uint64_t &N, int nworkers, size_t chunksize, bool on_demand=true) {
    auto make_farm = [&]() {
        std::vector<std::unique_ptr<ff::ff_node>> W;
        for(auto i = 0; i < nworkers; ++i)
            W.push_back(std::make_unique<Worker>());
        return W;
    };
    Emitter emitter(M, N, nworkers, chunksize);
    Collector collector(N);
    ff::ff_Farm<> farm(std::move(make_farm()), emitter, collector);
    farm.wrap_around();
    if(on_demand) 
        farm.set_scheduling_ondemand();
        
    if(farm.run_and_wait_end() < 0) {
        ff::error("running farm");
        return;
    }

    std::cout << "Total time (collector): " << collector.total_time << std::endl;
    std::cout << "Total time (emitter): " << emitter.total_time << std::endl;
    // print the time of each worker
    for(auto i = 0; i < nworkers; ++i) {
        std::cout << "Total time (worker " << i << "): " << static_cast<Worker*>(farm.getWorker(i))->total_time << std::endl;
    }

}

#endif // STENCIL_HPP
