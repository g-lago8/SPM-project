#include <mpi.h>
#include <stdio.h>
#include <vector>
#include<iostream>  
#include <iomanip>
#include <cmath>
#include <chrono>
#include <unistd.h> 

using namespace std;

void print_matrix(vector<vector<double>> &M){
    for (size_t i = 0; i < M.size(); i++){
        for (size_t j = 0; j < M.size(); j++){
            // cout << M[i][j] << " "; with 2 decimal points
            cout << fixed << setprecision(2) << M[i][j] << " ";
        }
        cout << endl;
    }
}

struct start_end {
    size_t start;
    size_t end;
};

start_end compute_start_end(int rank, int size, size_t N) {
    start_end se;
    size_t base_chunk = N / size;
    int remainder = N % size;
    se.start = rank * base_chunk + std::min(rank, remainder);
    se.end = se.start + base_chunk + (rank < remainder ? 1 : 0) - 1;
    return se;
}

void compute_internal_part(start_end se, size_t diag, vector<vector<double>> &M, size_t N) {
    if (se.start + 1 > se.end) return;
    for (auto row = se.start + 1; row < se.end; row++) {
        auto col = row + diag;
        double temp = 0;
        for (size_t j = 0; j < diag; j++) {
            temp += M[row][row + j] * M[col][col -j];
        }
        temp = cbrt(temp);
        M[col][row] = temp;
        M[row][col] = temp;
    }
}

void check_first(
    start_end se,
    size_t diag,
    size_t N,
    vector<vector<double>> &M,
    int rank,
    MPI_Request requests[2],
    bool * need_row,
    vector<double> &col_to_send,
    vector<double> &row_to_receive
){
     if (M[se.start][se.start+diag - 1] < 0){
        *need_row = true;
        MPI_Irecv(row_to_receive.data(), N, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &requests[1]);
    }
    else{
        for (size_t i = 0; i <diag; i++ ){
            col_to_send[i] = M[se.start + i ][se.start+diag - 1];
        }
        MPI_Isend(col_to_send.data(), N, MPI_DOUBLE, rank -1, 0, MPI_COMM_WORLD, &requests[1]);
    }
}

void check_last( 
    start_end se,
    size_t diag,
    size_t N, 
    vector<vector<double>> &M, 
    int rank, 
    MPI_Request requests[2], 
    bool *need_col,
    vector <double> &row_to_send,
    vector <double> &col_to_receive
){
    if (M[se.end +1][se.end + diag] < 0){
        *need_col =true;
        MPI_Irecv(col_to_receive.data(), N, MPI_DOUBLE, rank +1, 0, MPI_COMM_WORLD, &requests[0]) ;
    } else{
        for (size_t i= 0; i<diag;i++){
            row_to_send[i] = M[se.end + 1][se.end + i + 1];
        }
        MPI_Isend(row_to_send.data(), N,  MPI_DOUBLE, rank +1, 0, MPI_COMM_WORLD, &requests[0]);
    }
}

int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);
    auto start = chrono::high_resolution_clock::now();
    int rank, size;

    // argument check
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " N" << endl;
        return 1;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0)
        std::cout << "using " << size << " processes" <<std::endl;
    if (size < 2) {
        // abort if there is only one process
        cout << "This program is meant to be run with at least 2 processes" << endl;
        MPI_Finalize();
        return 1;
    }
    
    size_t N = atoi(argv[1]);
    vector<vector<double>> M(N, vector<double>(N, -1));
    auto se = compute_start_end(rank, size, N);

    for (size_t row = se.start; row <= se.end; row++) {
        M[row][row] = double(row +1)/N;
    }
    /*
    if (rank == 0){
        print_matrix(M);
        cout << endl;
    }
    */
    vector <double> row_to_send(N, -1);
    vector <double> row_to_receive(N, -1);
    vector <double> col_to_send(N, -1);
    vector <double> col_to_receive(N, -1);
    // initialize the buffers
    MPI_Request requests[2];
    for (size_t i = 0; i < 2; i++){
        requests[i] = MPI_REQUEST_NULL;
    }   
    bool need_row = false;
    bool need_col = false; // these get set to true when in the two if, the operation is a receive.

    for (size_t diag = 1; diag < N - 1; diag ++){
        se = compute_start_end(rank, size, N - diag); // compute first and last element to be processed by this process
        auto n_active_processes = min(size,int( N - diag ) + 1); // number of active processes (typically = size, but can be less for the last few iterations
        
        // first process does not need to check first
        if (rank == 0){
            if (n_active_processes > 1){
               check_last(se, diag, N, M, rank, requests, &need_col, row_to_send, col_to_receive); // check if we need the last column, or we need to give thelast row from/to the next process
            }
        }
        // last process does not need to check last element
        if ( rank == n_active_processes -1){
            check_first(se, diag, N, M, rank, requests, &need_row, col_to_send, row_to_receive); // check if we need the first row, or we need to give the first column from/to the previous process
        }

        // all other processes need to check both
        if (rank > 0 && rank < n_active_processes -1){
            check_first(se, diag, N, M, rank, requests, &need_row, col_to_send, row_to_receive); 
            check_last(se, diag, N, M, rank, requests, &need_col, row_to_send, col_to_receive);
        }

        compute_internal_part(se, diag, M, N); // compute the element in the middle of the chunk -> surely no dependencies
 
        auto rank_ull = static_cast<unsigned long long>(rank);
        if ( rank < n_active_processes && rank_ull < N -diag ){ // for each active process (except the last one in the case that the diag is shorter than the number of processes), compute the first and last element
            double temp = 0;
            size_t start_row =se.start;
            auto start_col = start_row + diag;
            if (need_row){ 
                MPI_Wait(&requests[1], MPI_STATUS_IGNORE); // wait for the row to be received
                for (size_t j =0; j <diag; j ++){
                    M[start_row][start_row + j] = row_to_receive[j]; // update the row
                    M[start_row + j][start_row] = row_to_receive[j]; // update the column simmetrically
                }
            }
            for (size_t j = 0; j < diag; j++) {
                temp += M[start_row][start_row + j] * M[start_col][start_col - j]; // compute the element
            }
            temp = cbrt(temp);
            M[start_row][start_col] = temp;
            M[start_col][start_row] = temp;

            // compute the last element
            auto end_row = se.end;
            auto end_col = end_row + diag;
            temp=0;
            if (need_col){
                MPI_Wait(&requests[0], MPI_STATUS_IGNORE); // wait for the column to be received
                for (size_t j = 0; j<diag; j ++ ){
                    M[end_row +j +1][end_col] = col_to_receive[j]; // update the column
                    M[end_col][end_row +j +1] = col_to_receive[j]; // update the symmetric row
                }
            }
            for (size_t j = 0; j < diag; j++) {
                temp += M[end_row][end_row + j] * M[end_col][end_col - j]; // compute the element
            }
            temp = cbrt(temp);
            M[end_row][end_col] = temp;
            M[end_col][end_row] = temp;

        }
        // reset the flags
        need_row = false;
        need_col = false;
        MPI_Barrier(MPI_COMM_WORLD);
    }

    // last step: compute the last element M[0][N-1] taking the missing column from process 1
    if (rank == 0){
        MPI_Recv(col_to_receive.data(), N, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // here I use a blocking receive because there is no possible computation overlap

        for (size_t j = 0; j < N - 1; j++) {
            M[j +1][N-1] = col_to_receive[j];
            M[N-1][j +1] = col_to_receive[j];

        }
        auto col = N -1;
        auto row = 0;
        double temp = 0;
        for (size_t j = 0; j < N - 1; j++) {
            temp += M[row][row + j] * M[col][col-j];
        }
        temp = cbrt(temp);
        M[col][row] =temp;
        M[row][col] = temp;
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Time: " << duration.count() << " ms" << endl;
    }
    
    if (rank == 1){
        for (size_t j = 0; j < N - 1; j++) {
            col_to_send[j] = M[j +1][N-1];
        }
        MPI_Send(col_to_send.data(), N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    if (rank==0 && N<11)
        print_matrix(M);
    

    MPI_Finalize();
    return 0;
}