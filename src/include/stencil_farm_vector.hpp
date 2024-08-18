#ifndef STENCIL_HPP_V
#define STENCIL_HPP_V

#include <iostream>
#include <vector>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <ff/utils.hpp>
#include <chrono>

void inline compute_stencil_naive(std::vector<double> &M, const uint64_t &N) {
    // naive implementation of the stencil computation, none of the optimizations are applied
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                M[i * N + i + diag]  += M[i * N + i + j] * M[(i + diag - j) * N + i + diag];
            }
            M[i * N + i + diag] = std::cbrt(M[i * N + i + diag]); // cube root
        }
    }
}

void inline compute_stencil_temp(std::vector<double> &M, const uint64_t &N) {
    // here, we accumulate the result in a temporary variable, to avoid writing to the same memory location multiple times.
    // In a sequential program, this would not change much, but in a parallel program, it can be faster, avoiding false sharing.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i * N + i + j] * M[(i_plus_diag - j) * N + i_plus_diag];
            }
            M[i * N + i_plus_diag] = temp;
            M[i * N + i_plus_diag] = std::cbrt(M[i * N + i_plus_diag]); // cube root
        }
    }
}

void inline compute_stencil_i_p_diag(std::vector<double> &M, const uint64_t &N) {
    // here we compute one time i_plus_diag =i+diag, that is used multiple times in the inner loop
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i * N + i + j] * M[(i_plus_diag - j) * N + i_plus_diag];
            }
            M[i * N + i_plus_diag] = temp;
            M[i * N + i_plus_diag] = std::cbrt(M[i * N + i_plus_diag]); // cube root
        }
    }
}

void inline compute_stencil_optim(std::vector<double> &M, const uint64_t &N) {
    // here we compute the stencil in a more cache-friendly way, by storing the result in the lower triangle, and copying it to the upper triangle, 
    // in order to do a dot product over two rows, instead of a dot product between a row and a column.
    for(uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for(uint64_t i = 0; i < (N-diag); ++i) {      // for each elem. in the diagonal
            auto i_plus_diag = i + diag;
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i * N + i + j] * M[i_plus_diag * N + i_plus_diag - j];
            }
            M[i_plus_diag * N + i] = temp;
            M[i_plus_diag * N + i] = std::cbrt(M[i_plus_diag * N + i]); // cube root
            M[i * N + i_plus_diag] = M[i_plus_diag * N + i]; // store the result also in the upper triangle
        }
    }
}

// ------------------------------------------------------------------
// ---------------------- FARM IMPLEMENTATION -----------------------
// ------------------------------------------------------------------
struct Task {
    size_t diag;
    size_t row;
    size_t chunksize;
};

void inline compute_stencil_one_chunk(
    // Function used by the workers in the farm, computes a piece of the wavefront.
    std::vector<double> &M,
    const uint64_t &N,
    const uint64_t &diag,
    uint64_t &row,
    const uint64_t &chunksize) 
{
    size_t end = std::min(row + chunksize, N-diag);
    for(; row < end; ++row) {
        double temp = 0;
        auto col = row + diag; 
        for(uint64_t j = 0; j < diag; ++j) {
            temp += M[row * N + row + j] * M[col * N + col - j]; // dot product. 
        }

        // we store the result in the lower triangle, to do a dot product over two rows, 
        //instead of a dot product between a row and a column (better cache locality)
        M[col * N + row] = temp; 
        M[col * N + row] = std::cbrt(M[col * N + row]); // cube root
        M[row * N + col] = M[col * N + row]; // store the result also in the upper triangle
    }
}

struct Emitter: ff::ff_monode_t<bool, Task> {
    Emitter(std::vector<double> &M, size_t N, int n_workers,  size_t chunksize = 1): M(M), N(N), n_workers(n_workers), chunksize(chunksize) {}
    size_t diag = 1;
    double total_time;

    Task* svc(bool *diagonal_is_done) {
        // send the tasks to the workers
        if( n_workers * chunksize > N - diag) {
            chunksize = N / n_workers; // if the chunksize is too big, we reduce it
        }

        if(diagonal_is_done != nullptr) {
            *diagonal_is_done = false; // reset the signal received by the collector
        }
        
        for(uint64_t row = 0; row < (N - diag); row += chunksize) {      // for each elem. in the diagonal
            size_t block_size = std::min(N - diag - row, chunksize); // the last chunk might be smaller
            Task *task = new Task{diag, row, block_size};
            ff_send_out(task);
        }
        diag++;
        if (diag == N) return EOS;
        return GO_ON;
    }

    std::vector<double> &M;
    size_t N;
    int n_workers;
    size_t chunksize;
};

struct Worker: ff::ff_node_t<Task, Task> {
    std::vector<double> &M;
    size_t N;
    Worker(std::vector<double> &M, size_t N): M(M), N(N) {}
    Task* svc(Task *task) {
        compute_stencil_one_chunk(M, N, task->diag, task->row, task->chunksize);
        return task;
    }
};

struct Collector: ff::ff_minode_t<Task, bool> {
    Collector(size_t N): N(N) {}
    bool* svc(Task *computed) {
        done += computed->chunksize; // update the number of elements computed
        delete computed;
        if(done == N - diag) { // if the diagonal is all done
            done = 0;
            diag++;
            diagonal_is_done = true;
            return &diagonal_is_done; // send the signal to the emitter
        }
        return GO_ON; // else do nothing and keep going
    }

    size_t done = 0;
    size_t N;
    size_t diag = 1;
    bool diagonal_is_done = false;
};

void compute_stencil_par(std::vector<double> &M, const uint64_t &N, int nworkers, size_t chunksize, bool on_demand=true) {
    auto make_farm = [&]() {
        std::vector<std::unique_ptr<ff::ff_node>> W;
        for(auto i = 0; i < nworkers; ++i)
            W.push_back(std::make_unique<Worker>(M, N));
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
}

#endif // STENCIL_HPP