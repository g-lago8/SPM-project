#include <mpi.h>
#include <stdio.h>
#include <vector>
#include<iostream>  
#include <iomanip>
#include <cmath>
#include <chrono>
#include <unistd.h> // Add this line to include the sleep function
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
            temp += M[row][row + j] * M[col - j][col];
        }
        temp = cbrt(temp);
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
        cout << "rank " << rank <<" sending " ;
        for (auto i{0}; i <diag; i++ ){
            col_to_send[i] = M[se.start + i ][se.start+diag - 1];
             cout << col_to_send[i] << " ";
        }
        cout << endl;
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
    }
    else{
        for (auto i{0}; i<diag;i++){
            row_to_send[i] = M[se.end + 1][se.end + i + 1];
        }
        MPI_Isend(row_to_send.data(), N,  MPI_DOUBLE, rank +1, 0, MPI_COMM_WORLD, &requests[0]);
    }
}

int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    size_t N = 7;
    vector<vector<double>> M(N, vector<double>(N, -1));
    auto se = compute_start_end(rank, size, N);

    for (size_t row = se.start; row <= se.end; row++) {
        M[row][row] = double(row +1 )/N;
    }

    vector <double> row_to_send(N, -1);
    vector <double> row_to_receive(N, -1);
    vector <double> col_to_send(N, -1);
    vector <double> col_to_receive(N, -1);
    // initialize the buffers
    MPI_Request requests[2];
    for (size_t i = 0; i < 2; i++){
        requests[i] = MPI_REQUEST_NULL;
    }   
    MPI_Request req;
    bool need_row, need_col = false; // these get set to true when in the two if, the operation is a receive. 


    // -------------- MAIN LOOP -----------------------------------------------------------------------------------
    // 
    // ------------------------------------------------------------------------------------------------------------

    for (size_t diag = 1; diag < N - 1; diag ++){
        se = compute_start_end(rank, size, N - diag); // compute first and last element to be processed by this process
        auto n_active_processes = min(size,int( N - diag )); // number of active processes (typically = size, but can be less for the last few iterations
        
        // first process does not need to check first
        if (rank == 0){
            if (n_active_processes > 1){
               check_last(se, diag, N, M, rank, requests, &need_col, row_to_send, col_to_receive); // check if we need the last column, or we need to give thelast row from/to the next process
            }
        }

        // last process does not need to check last
        if (rank == n_active_processes -1){
            if (n_active_processes > 1){
                check_first(se, diag, N, M, rank, requests, &need_row, col_to_send, row_to_receive); // check if we need the first row, or we need to give the first column from/to the previous process
            }
        }

        // all other processes need to check both
        if (rank > 0 && rank < n_active_processes -1){
            check_first(se, diag, N, M, rank, requests, &need_row, col_to_send, row_to_receive); 
            check_last(se, diag, N, M, rank, requests, &need_col, row_to_send, col_to_receive);
        }
        compute_internal_part(se, diag, M, N); // compute the element in the middle of the chunk -> surely no dependencies
 

        if ( rank < n_active_processes){ // for each active process, compute the first and last element
            
            double temp = 0;
            size_t start_row =se.start;
            auto start_col = start_row + diag;
            if (need_row){ 
                cout << "request row "<< &requests[1] << " from " << rank << endl;
                MPI_Wait(&requests[1], MPI_STATUS_IGNORE); // wait for the row to be received
                cout << "rank " << rank << " received the row" << endl;
                for (size_t j =0; j <diag; j ++)
                    M[start_row][start_row + j] = row_to_receive[j]; // update the row
            }
            for (size_t j = 0; j < diag; j++) {
                temp += M[start_row][start_row + j] * M[start_col - j][start_col]; // compute the element
            }
            temp = cbrt(temp);
            M[start_row][start_col] = temp;

            cout <<"diag " << diag << " rank " << rank << " computed the first element" << "(" << start_row << "," << start_col << ")" << endl;
            // compute the last element
            auto end_row = se.end;
            auto end_col = end_row + diag;
            temp=0;
            if (need_col){
                cout << "request col "<< &requests[0] << " from " << rank << endl;   
                MPI_Wait(&requests[0], MPI_STATUS_IGNORE); // wait for the column to be received
                cout << "rank " << rank << " received the column" << endl;
                for (auto j = 0; j<diag; j ++ ){
                    cout << col_to_receive[j] << " ";
                    M[end_row +j +1][end_col] = col_to_receive[j]; // update the column
                }
                cout << endl;
            }
            for (size_t j = 0; j < diag; j++) {
                temp += M[end_row][end_row + j] * M[end_col - j][end_col]; // compute the element
            }
            temp = cbrt(temp);
            M[end_row][end_col] = temp;
            cout << "diag " << diag << " rank " << rank << " computed the last element" << "(" << end_row << "," << end_col << ")" << endl;
        }
        if (rank == 2){
            print_matrix(M);
            cout << endl;
        }
        // reset the flags
        need_row = false;
        need_col = false;
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if (rank == 0) cout << "final matrix" << endl;
    // last step: compute the last element taking the missing column from process 1


    if (size == 1){
        if (rank == 0){       double temp = 0;
            for (size_t j = 0; j < N - 1; j++) {
                temp += M[0][j] * M[N - 1 - j][N - 1];
            }
            temp = cbrt(temp);
            M[0][N - 1] = temp;
            print_matrix(M);
        }
    } else { // more than 1 process
        if (rank == 0){
            MPI_Recv(col_to_receive.data(), N, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // here I use a blocking receive because there is no possible computation overlap
            cout << "rank " << rank << " received the column" << endl;  

            cout << endl;
            cout << endl;
            for (size_t j = 0; j < N - 1; j++) {
                M[j +1][N-1 ] = col_to_receive[j];
                cout << M[j +1][N-1] << " ";
            }
            cout << endl;
            cout << "--------------------------------" << endl;
            auto col = N -1;
            auto row = 0;
            double temp = 0;
            for (size_t j = 0; j < N - 1; j++) {
                temp += M[row][row + j] * M[col - j][col];
            }
            temp = cbrt(temp);
            M[row][col] = temp;
            print_matrix(M);
        }
        
        if (rank == 1){
            for (size_t j = 0; j < N - 1; j++) {
                col_to_send[j] = M[j +1][N-1];
            }
            MPI_Send(col_to_send.data(), N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
    return 0;
}