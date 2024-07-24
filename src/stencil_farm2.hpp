#ifndef STENCIL_HPP
#define STENCIL_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <ff/utils.hpp>
#include <ff/node.hpp>
#include <chrono>
#include <utility>




// ------------------------------------------------------------------
// ---------------------- FARM IMPLEMENTATION -----------------------
// ------------------------------------------------------------------

// define a block of elements as a pair of start and end indices
std::pair<size_t, size_t> compute_start_end(size_t diag, const size_t &N, size_t worker_id, size_t n_workers) {
    size_t base_chunk = N / n_workers;
    size_t remainder = N % n_workers;
    size_t start = worker_id * base_chunk + std::min(worker_id, remainder);
    size_t end = start + base_chunk + (worker_id < remainder ? 1 : 0) - 1;
    return {start, end};
}

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

struct Emitter: ff::ff_monode_t<bool, size_t>{
    Emitter(std::vector<std::vector<double>> &M, size_t N, int n_workers):M(M), N(N), n_workers(n_workers) {}
    size_t diag =0;
    double total_time;

    size_t* svc(bool *diagonal_is_done){
        diag ++;
        // send the tasks to the workers
        if(diagonal_is_done!=nullptr){
            *diagonal_is_done = false; // reset the signal received by the collector
        }
        
        for(int nw = 0; nw < n_workers; nw ++){      // for each elem. in the diagonal
            ff_send_out(&diag);
        }
        if (diag == N -1) return EOS;
        return GO_ON;
    }


    std::vector<std::vector<double>> &M;
    size_t N;
    int n_workers;
};


struct Worker: ff::ff_node_t<size_t, int> {
    std::vector<std::vector<double>> &M;
    size_t N;
    int n_workers;
    Worker(std::vector<std::vector<double>> &M, size_t N, int n_workers): M(M), N(N), n_workers(n_workers) {}
    int* svc(size_t *diag)  {
        auto block = compute_start_end( *diag, N, get_my_id(), n_workers);
        compute_stencil_one_chunk(M, N, *diag, block.first, block.second - block.first + 1);
        return new int{1};
    }
};


struct Collector: ff::ff_minode_t<int, bool> {
    Collector(size_t N, int n_workers): N(N), n_workers(n_workers) {}
    bool* svc(int *computed) {
        done += 1; // update the number of elements computed
        delete computed;
        if(done == n_workers ) { // if the diagonal is all done
            done = 0;
            diag++;
            diagonal_is_done = true;
            return &diagonal_is_done; // send the signal to the emitter
        }
        return GO_ON; // else do nothing and keep going
    }
    int done = 0;
    size_t N;
    size_t diag = 1;
    bool diagonal_is_done = false;
    int n_workers;
};


void compute_stencil_par(std::vector<std::vector<double>> &M, const uint64_t &N, int nworkers, bool on_demand=false) {
    auto make_farm = [&]() {
        std::vector<std::unique_ptr<ff::ff_node>> W;
        for(auto i = 0; i < nworkers; ++i)
            W.push_back(std::make_unique<Worker>(M, N, nworkers));
        return W;
    };
    Emitter emitter(M, N, nworkers);
    Collector collector(N, nworkers);
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
