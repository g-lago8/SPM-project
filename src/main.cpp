#include<ff/ff.hpp>
#include<ff/farm.hpp>
#include <iostream>
#include <random>
#include <vector>
using namespace ff;

using task_t=std::pair<float, size_t>;
const size_t minVsize = 512;
const size_t maxVsize = 8192;

// a simple pipeline
// emitter

struct Emitter: ff_node_t<task_t> {
    Emitter() {}
    // print a message when the emitter starts
    int svc_init() {
        std::cout << "Emitter started" << std::endl;
        return 0;
    }
    // function that sends out 10 numbers
    task_t* svc(task_t *task) {
        for(int i=0; i<10; ++i) {
            ff_send_out(new int(i));
        }
        return task;
    }
    // print a message when the emitter ends
    void svc_end() {
        std::cout << "Emitter ended" << std::endl;
    }
};



// worker
struct Worker: ff_node_t<task_t> {
    Worker() {}
    // print a message when the worker receives a number
    task_t* svc(task_t* task) {
        int* number = (int*)task;
        std::cout << "Received number: " << *number << std::endl;
        delete number;
        return task;
    }
    // print a message when the worker ends
    void svc_end() {
        std::cout << "Worker ended" << std::endl;
    }
};

// collector

struct Collector: ff_node_t<float> {
    Collector() {}
    float* svc(task_t* task) {

        std::cout << "Received result: "  << task->first << std::endl;
        delete task;

        return GO_ON;
    }
    
    void svc_end() {
        std::cout << "Collector ended" << std::endl;
    }
};

int main() {
    // create the pipeline
    ff_pipeline pipe;
    Emitter emitter;
    Worker worker;
    Collector collector;

}