// tests strong scaling of the parallel implementation
#include "stencil_farm.hpp"
#include <chrono>
#include <iostream>
using namespace std;

vector<double> calculate_mean(vector<vector<double>>& results, int n_tries, vector<int>& nworkers) {
    vector <double> means(nworkers.size(), 0.0);
    for (auto j = 0; j < nworkers.size(); j++){
        double sum = 0;
        double min = results[j][0];
        double max = results[j][0];
        for (auto i = 0; i < n_tries; i++){
            sum += results[j][i];
            if (results[j][i] < min){
                min = results[j][i];
            }
            if (results[j][i] > max){
                max = results[j][i];
            }
        }
        sum -= min;
        sum -= max;
        means[j] = sum / (n_tries - 2);
    }
    return means;
}

int main(int argc, char *argv[]){
    size_t N = 2048; // default size of the matrix (NxN)   
    int n_tries = 10; // default number of tries
    if (argc >3){
        cout << "use: " << argv[0] << " [N, n_tries]"<<endl;
        cout << "     N: size of the square matrix\n";
    }
    if (argc > 1){
        N = stol(argv[1]);
    }
    if (N < 1){
        cout << "Error: N must be greater than 0" << endl;
        return -1;
    }
    vector<vector<double>> M(N, vector<double>(N, 0.0));
    for (size_t i = 0; i < N; ++i){
        M[i][i] = double(i+1)/double(N);
    }

    vector <int> nworkers = {1, 2, 4, 6, 8, 10, 12, 16, 20, 24};
    vector<vector<double>> results (nworkers.size(), vector<double>(n_tries, 0.0));
    for (auto j = 0; j < nworkers.size(); j++){
        cout << "nworkers: " << nworkers[j] << endl;
        for (auto i =0 ; i< n_tries; i++){
            auto M1 = M;
            auto nw = nworkers[j];
            auto start = chrono::steady_clock::now();
            compute_stencil_par(M1, N, nw, 1);
            auto end = chrono::steady_clock::now();
            chrono::duration<double> elapsed_seconds = end-start;
            cout << "elapsed time: " << elapsed_seconds.count() << "s\n";
            results[j][i] = elapsed_seconds.count();
        }
    }
    // mean of the results except lowest and greatest value
    
    auto means = calculate_mean(results, n_tries, nworkers);

    // save files in a csv format
    ofstream file;
    file.open("strong_scaling.csv");
    file << "nworkers,mean_time\n";
    for (auto i = 0; i < nworkers.size(); i++){
        file << nworkers[i] << "," << means[i] << "\n";
    }
    file.close();
    return 0;
    
}