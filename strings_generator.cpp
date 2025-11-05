#include <string>
#include <random>
#include <fstream>
#include <iostream>


std::string stringGenerator(int length, const std::string& alphabet = "ACGT")
{
    std::string result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, alphabet.size() - 1);

    for(int i=0; i < length; i++){
        result += alphabet[dis(gen)];
    }

    return result;
}

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cerr << "Usage: ./generator <length>\n";
        return 1;
    }

    int length = std::stoi(argv[1]);
    std::string A = stringGenerator(length);
    std::string B = stringGenerator(length);

    std::string fileA = "inputA_"+std::to_string(length)+".txt";
    std::string fileB = "inputB_"+std::to_string(length)+".txt";

    std::ofstream outA(fileA);
    std::ofstream outB(fileB);

    if(!outA || !outB){
        std::cerr << "Error to writing files. \n";
        return 1;
    }

    outA << A;
    outB << B;

    outA.close();
    outB.close();

    std::cout << "Generated two random DNA-like strings of length " << length << ":\n";
    std::cout << "A (first 20 chars): " << A.substr(0, std::min(20, length))
              << (length > 20 ? "..." : "") << "\n";
    std::cout << "B (first 20 chars): " << B.substr(0, std::min(20, length))
              << (length > 20 ? "..." : "") << "\n";
    std::cout << "Saved to: " << fileA << " and " << fileB << std::endl;

}