#ifndef STENCIL_HPP
#define STENCIL_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <ff/utils.hpp>
#include <chrono>

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
    std::vector<std::vector<double>> &M,
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
            temp += M[row][row+j] * M[col][col-j]; // dot product. 
        }

        // we store the result in the lower triangle, to do a dot product over two rows, 
        //instead of a dot product between a row and a column (better cache locality)
        M[col][row] =temp; 
        M[col][row] = std::cbrt(M[col][row]); // cube root
        M[row][col] = M[col][row]; // store the result also in the upper triangle

    }
}

struct Emitter: ff::ff_monode_t<bool, Task>{
    Emitter(std::vector<std::vector<double>> &M, size_t N, int n_workers,  size_t chunksize = 1):M(M), N(N), n_workers(n_workers), chunksize(chunksize) {}
    size_t diag =1;
    double total_time;

    Task* svc(bool *diagonal_is_done){
        // send the tasks to the workers
        if( n_workers * chunksize > N - diag){
            chunksize = N / n_workers; // if the chunksize is too big, we reduce it
        }

        if(diagonal_is_done!=nullptr){
            *diagonal_is_done = false; // reset the signal received by the collector
        }
        
        for(uint64_t row = 0; row< (N- diag); row += chunksize){      // for each elem. in the diagonal
            size_t block_size = std::min( N-diag-row, chunksize); // the last chunk might be smaller
            Task *task = new Task{diag, row, block_size};
            // print the size of task in bytes
            //std::cout << "Task size: " << sizeof(*task) << " bytes" << std::endl;
            ff_send_out(task);
        }
        diag++;
        if (diag == N) return EOS;
        return GO_ON;
    }


    std::vector<std::vector<double>> &M;
    size_t N;
    int n_workers;
    size_t chunksize;
};

struct Worker: ff::ff_node_t<Task, Task> {
    std::vector<std::vector<double>> &M;
    size_t N;
    Worker(std::vector<std::vector<double>> &M, size_t N): M(M), N(N) {}
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
        if(done == N-diag) { // if the diagonal is all done
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


void compute_stencil_par(std::vector<std::vector<double>> &M, const uint64_t &N, int nworkers, size_t chunksize, bool on_demand=true) {
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
