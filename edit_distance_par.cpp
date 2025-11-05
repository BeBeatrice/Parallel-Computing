#include <mpi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;


int get_cost(char a, char b){
    return (a==b) ? 0 : 1;
}

int computeParED(const string &A, const string &B, int rank, int size) {
    
    int n = A.size();
    int m = B.size();
    int tot_diags = n + m; 
    
    // We use two vectors to store diag - 1 (previous diagonal) and diag -2 (previous previous diagonal)
    
    vector<int> prev_diag(n + 1, 0); 
    vector<int> prev_prev_diag(n + 1, 0); 

    if (rank == 0) {
        // D[i, 0] = i (Col 0)
        for (int i = 0; i <= n; ++i) {
            prev_diag[i] = i;
            prev_prev_diag[i] = i; 
        }
        prev_diag[0] = 0; // D[0, 0]
        prev_prev_diag[0] = 0; // D[0, 0]
    }
    
    // Cast the two vectors to all ranks
    MPI_Bcast(prev_diag.data(), n + 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(prev_prev_diag.data(), n + 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Stores results of the current diagonal
    vector<int> current_diag_results(n + 1, 0);

    //  Each diagonal is processed sequentially
    for (int diag = 1; diag <= tot_diags; ++diag) {

        int i_start = std::max(1, diag - m);
        int i_end   = std::min(n, diag - 1); 
        int current_diag_len = i_end - i_start + 1;

        if (current_diag_len <= 0) {
            MPI_Barrier(MPI_COMM_WORLD);
            continue;
        }

        // the values inside each diagonal are computed in parallel: to each rank we assign a chunk of the diagonal
        int chunk_size = current_diag_len / size;
        int remainder  = current_diag_len % size;
        int local_start_offset = rank * chunk_size + std::min(rank, remainder);
        int local_chunk_size   = chunk_size + (rank < remainder ? 1 : 0);
        int local_i_start = i_start + local_start_offset;
        int local_i_end   = local_i_start + local_chunk_size - 1;

        //  stores the results computed from the rank for the current diagonal
        vector<int> local_results_chunk; 
        int val_left_received = 0;
        
        // Receive the left value from the previous rank (rank-1) if needed
        if (rank > 0 && local_chunk_size > 0) {
            MPI_Recv(&val_left_received, 1, MPI_INT, rank - 1, diag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Local computing
        for (int i = local_i_start; i <= local_i_end; ++i) {
            int j = diag - i;
            
            // D[i-1, j] (up value from diag-1)
            int val_up = (i > 0) ? prev_diag[i - 1] : j; // base case: D[0, j] = j
            
            // D[i-1, j-1] (Diagonal from diag-2)
            int val_diag = (i > 0) ? prev_prev_diag[i - 1] : j - 1; // base case: D[0, j-1] = j-1
            
            // D[i, j-1] (left value from diag-1)
            int val_left;
            if (j == 0) {
                val_left = i; // D[i, 0] = i
            } else if (i == local_i_start && rank > 0) {
                val_left = val_left_received;
            } else if (i == local_i_start && rank == 0) {
                val_left = prev_diag[i]; // D[i, j-1]
            } else {
                // computed by this rank in the previous iteration
                val_left = current_diag_results[i - 1];
            }

            int cost = get_cost(A[i - 1], B[j - 1]);

            int new_val = std::min({
                val_up + 1,
                val_left + 1,
                val_diag + cost
            });

            current_diag_results[i] = new_val;
            local_results_chunk.push_back(new_val);
        }

        // send the rightmost value to the next rank (rank+1) if needed
        if (rank < size - 1 && local_chunk_size > 0) {
            int value_to_send = current_diag_results[local_i_end];
            MPI_Send(&value_to_send, 1, MPI_INT, rank + 1, diag, MPI_COMM_WORLD);
        }

        //  Gather all local results to form the complete diagonal
        vector<int> recvcounts(size);
        vector<int> displs(size);
        int local_data_size = local_results_chunk.size();
        MPI_Allgather(&local_data_size, 1, MPI_INT, recvcounts.data(), 1, MPI_INT, MPI_COMM_WORLD);
        
        displs[0] = 0;
        int total_received = recvcounts[0];
        for (int i = 1; i < size; ++i) {
            displs[i] = displs[i - 1] + recvcounts[i - 1];
            total_received += recvcounts[i];
        }
        vector<int> global_diag_results(total_received);
        MPI_Allgatherv(local_results_chunk.data(), local_data_size, MPI_INT,
                       global_diag_results.data(), recvcounts.data(), displs.data(),
                       MPI_INT, MPI_COMM_WORLD);

        // Update prev_prev_diag and prev_diag for the next iteration
        // prev_prev_diag becomes obsolete
        // prev_diag becomes prev_prev_diag
        // current becomes prev_diag
        
        prev_prev_diag = prev_diag; Copia
        
        int k_idx = 0;
        for (int i = i_start; i <= i_end; ++i) {
            prev_diag[i] = global_diag_results[k_idx++];
        }

        if (diag <= m) { 
            prev_diag[0] = diag; 
        }
    }

    return prev_diag[n];
}
int main(int argc, char* argv[]){

    // Initialize MPI
    MPI_Init(&argc, &argv); 

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    std::string stringA, stringB;

    if (rank == 0) {
        if(argc < 3){
            std::cerr << "Usage: ./edit_distance_par <fileA> <fileB>\n";
            MPI_Abort(MPI_COMM_WORLD, 1); // Uscita pulita in caso di errore
        }

        std::ifstream fileA(argv[1]);
        std::ifstream fileB(argv[2]);

        if(!fileA || !fileB){
            std::cerr << "Error opening one or both files." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        std::string stringA;
        std::string stringB;
        std::getline(fileA, stringA);
        std::getline(fileB, stringB);

        fileA.close();
        fileB.close();
    }    
  
    int lenA = (rank == 0) ? stringA.length() : 0;
    int lenB = (rank == 0) ? stringB.length() : 0;
    
    // 1. Bcast delle lunghezze
    MPI_Bcast(&lenA, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&lenB, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        stringA.resize(lenA);
        stringB.resize(lenB);
    }
    
    if (lenA > 0) {
        MPI_Bcast(const_cast<char*>(stringA.data()), lenA, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    if (lenB > 0) {
        MPI_Bcast(const_cast<char*>(stringB.data()), lenB, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    
    double start_time = MPI_Wtime();
    int result = computeParED(stringA, stringB, rank, size);
    
    MPI_Barrier(MPI_COMM_WORLD); 
    double end_time = MPI_Wtime();

    if (rank == 0) { 
        std::cout << "Edit Distance: " << result << std::endl;
        std::cout << "Execution Time (" << size << " processes): " << end_time - start_time << " seconds" << std::endl; 
    }
    
    MPI_Finalize();
    return 0;
}