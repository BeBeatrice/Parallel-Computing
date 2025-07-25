#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>

int computeSeqED(const std::string &A, const std::string &B){

    int n = A.size();
    int m = B.size();

    //  create the DP matrix
    int rows = n+1;
    int cols = m+1;
    std::vector<std::vector<int>> dp_matrix(rows, std::vector<int>(cols, 0));

    //  base cases
    for (int i = 0; i < rows ; ++i){
        dp_matrix[i][0] = i;
    }
    for (int j = 0; j < cols; ++j){
        dp_matrix[0][j] = j;
    }

    for(int i = 1; i < rows; i++){
        for(int j = 1; j < cols; j++){
            //  match
            if (A[i-1] == B[j-1]) {
                dp_matrix[i][j] = dp_matrix[i-1][j-1];
            } else {
                // mismatch
                dp_matrix[i][j] = std::min({    dp_matrix[i-1][j-1]+1,  //  substitution
                                                dp_matrix[i][j-1]+1,    //  insertion
                                                dp_matrix[i-1][j]+1     //  deletion
                                            });
            }
        }
    }

    return dp_matrix[n][m];
}

int main(int argc, char* argv[]){

    if(argc < 3){
        std::cerr << "Usage: ./edit_distance_seq <fileA> <fileB>\n";
        return 1;
    }

    std::ifstream fileA(argv[1]);
    if(!fileA){
        std::cerr << "Error opening file A" << std::endl;
        return 1;
    }

    std::ifstream fileB(argv[2]);
    if(!fileB){
        std::cerr << "Error opening file B" << std::endl;
        return 1;
    }

    std::string stringA;
    std::string stringB;
    std::getline(fileA, stringA);
    std::getline(fileB, stringB);

    fileA.close();
    fileB.close();
    
    auto start = std::chrono::high_resolution_clock::now();
    int result = computeSeqED(stringA, stringB);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end-start;
    std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
    return 0;
}