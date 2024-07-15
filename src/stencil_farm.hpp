#ifndef STENCIL_HPP
#define STENCIL_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <ff/utils.hpp>

struct Task {
    std::vector<double>& M;
    size_t N;
    size_t diag;
    size_t i;
    size_t chunksize;
};

void inline compute_stencil_one_chunk(
    std::vector<double>& M,
    const uint64_t& N,
    const uint64_t& diag,
    uint64_t& i,
    const uint64_t& chunksize)
{
    size_t end = std::min(i + chunksize, N - diag);
    for (; i < end; ++i) {
        double temp = 0;
        for (uint64_t j = 0; j < diag; ++j) {
            temp += M[i * N + i + j] * M[(i + diag - j) * N + i + diag];
        }
        M[i * N + i + diag] = temp;
        M[i * N + i + diag] = std::cbrt(M[i * N + i + diag]);
    }
}

void inline compute_stencil(std::vector<double>& M, const uint64_t& N) {
    for (uint64_t diag = 1; diag < N; ++diag) {        // for each upper diagonal
        for (uint64_t i = 0; i < (N - diag); ++i) {      // for each elem. in the diagonal
            double temp = 0.0;
            for (uint64_t j = 0; j < diag; ++j) {     // for each elem. in the stencil
                temp += M[i * N + i + j] * M[(i + diag - j) * N + i + diag];
            }
            M[i * N + i + diag] = temp;
            M[i * N + i + diag] = std::cbrt(M[i * N + i + diag]);
        }
    }
}

struct Emitter : ff::ff_monode_t<bool, Task> {
    Emitter(std::vector<double>& M, size_t N, int n_workers, size_t chunksize = 1) : M(M), N(N), n_workers(n_workers), chunksize(chunksize) {}
    size_t diag = 1;
    Task* svc(bool* diagonal_is_done) {

        // send the tasks to the workers
        if (n_workers * chunksize > N - diag) {
            chunksize = N / n_workers;
        }
        for (uint64_t i = 0; i < (N - diag); i += chunksize) {      // for each elem. in the diagonal
            size_t block_size = std::min(N - diag - i, chunksize);
            if (diagonal_is_done != nullptr) {
                *diagonal_is_done = false;
            }
            ff_send_out(new Task{ M, N, diag, i, block_size });
        }
        diag++;
        if (diag == N) return EOS;
        return GO_ON;
    }

    std::vector<double>& M;
    size_t N;
    int n_workers;
    size_t chunksize;
};

struct Worker : ff::ff_monode_t<Task, Task> {
    Task* svc(Task* task) {
        compute_stencil_one_chunk(task->M, task->N, task->diag, task->i, task->chunksize);
        return task;
    }
};

struct Collector : ff::ff_minode_t<Task, bool> {
    Collector(size_t N) : N(N) {}

    bool* svc(Task* computed) {
        done += computed->chunksize;
        delete computed;
        if (done == N - diag) { // if the diagonal is all done
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

void compute_stencil_par(std::vector<double>& M, const uint64_t& N, int nworkers, size_t chunksize, bool on_demand = true) {
    auto make_farm = [&]() {
        std::vector<std::unique_ptr<ff::ff_node>> W;
        for (auto i = 0; i < nworkers; ++i)
            W.push_back(std::make_unique<Worker>());
        return W;
    };
    Emitter emitter(M, N, nworkers, chunksize);
    Collector collector(N);
    ff::ff_Farm<> farm(std::move(make_farm()), emitter, collector);
    farm.wrap_around();
    if (on_demand)
        farm.set_scheduling_ondemand();

    if (farm.run_and_wait_end() < 0) {
        ff::error("running farm");
        return;
    }
}

#endif // STENCIL_HPP
