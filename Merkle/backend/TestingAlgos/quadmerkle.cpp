/*
    ALGORITHM 3: Quad Merkle Tree (4-ary)
    Logic: Reduces Tree Height by half (Log base 4 instead of Log base 2).
    Complexity: Still O(N), but fewer total hashing operations for internal nodes.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono> 
#include <cmath>

#include "sha.hpp"
#include "json.hpp" 

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

struct Review {
    string id;          
    string rawData;     
};

class QuadMerkleTree {
private:
    vector<vector<string>> levels; 

public:
    string rootHash;

    QuadMerkleTree(const vector<Review>& data) {
        vector<string> leaves;
        
        // 1. Hash Leaves
        for(const auto& item : data) {
            leaves.push_back(sha256(item.rawData));
        }
        levels.push_back(leaves);
        
        // 2. Build Tree (Quad Logic)
        buildTree();
    }

    void buildTree() {
        vector<string> currentLevel = levels[0];

        while (currentLevel.size() > 1) {
            vector<string> nextLevel;
            
            // Step by 4 instead of 2
            for (size_t i = 0; i < currentLevel.size(); i += 4) {
                string combined = currentLevel[i];

                // Append Child 2 (if exists)
                if (i + 1 < currentLevel.size()) combined += currentLevel[i+1];
                
                // Append Child 3 (if exists)
                if (i + 2 < currentLevel.size()) combined += currentLevel[i+2];

                // Append Child 4 (if exists)
                if (i + 3 < currentLevel.size()) combined += currentLevel[i+3];

                // Hash all 4 children together
                nextLevel.push_back(sha256(combined));
            }
            
            levels.push_back(nextLevel);
            currentLevel = nextLevel;
        }
        rootHash = currentLevel[0];
    }

    string getRoot() { return rootHash; }
};

int main() {
    string filePath = "../Dataset/Musical_Instruments.json";
    vector<Review> dataset;
    dataset.reserve(1600000); 

    cout << "Loading Dataset..." << endl;
    ifstream file(filePath);
    if (!file.is_open()) { cerr << "Error: File not found!" << endl; return 1; }

    string line;
    while (getline(file, line)) {
        try {
            json j = json::parse(line);
            Review r;
            r.id = j.value("asin", "UNK");
            r.rawData = r.id + to_string(j.value("overall", 0.0)) + j.value("reviewText", "");
            dataset.push_back(r);
        } catch (...) { continue; }
    }
    cout << "Dataset Loaded: " << dataset.size() << endl;

    cout << "Starting QUAD Tree Construction (4 children)..." << endl;
    
    auto start = high_resolution_clock::now();
    
    // --- ALGORITHM START ---
    QuadMerkleTree* tree = new QuadMerkleTree(dataset);
    // --- ALGORITHM END ---

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Construction Completed." << endl;
    cout << "Merkle Root: " << tree->getRoot() << endl;
    cout << "------------------------------------------------" << endl;
    cout << "QUAD TREE TIME: " << duration.count() << " ms" << endl;
    cout << "------------------------------------------------" << endl;

    return 0;
}