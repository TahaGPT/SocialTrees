/*
    BASELINE ALGORITHM: Standard Serial Merkle Tree
    Complexity: O(N) Time | O(N) Space
    Logic: 
      1. Hash all data (Leaves).
      2. Pair (i, i+1).
      3. Hash(Left + Right).
      4. Repeat until Root.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono> 
#include <unordered_map>

// Reuse your existing headers
#include "sha.hpp"
#include "json.hpp" 

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

// ==========================================
// Data Structure
// ==========================================
struct Review {
    string id;          
    string rawData;     
};

// ==========================================
// Class: Basic Merkle Tree (The Baseline)
// ==========================================
class BasicMerkleTree {
private:
    vector<string> leaves; 
    vector<vector<string>> levels; 

public:
    string rootHash;

    // Constructor initiates the build
    BasicMerkleTree(const vector<Review>& data) {
        // 1. Hash Leaves (Serial)
        for(const auto& item : data) {
            leaves.push_back(sha256(item.rawData));
        }

        levels.push_back(leaves);
        
        // 2. Build Tree (Serial)
        buildTree();
    }

    void buildTree() {
        vector<string> currentLevel = leaves;

        // Loop until we reach the single Root
        while (currentLevel.size() > 1) {
            vector<string> nextLevel;
            
            // Algorithmic Logic: Pair adjacent nodes
            for (size_t i = 0; i < currentLevel.size(); i += 2) {
                string left = currentLevel[i];
                
                // Handle Odd Number Case: Duplicate the last node
                string right = (i + 1 < currentLevel.size()) ? currentLevel[i + 1] : left;
                
                // Combined Hash
                nextLevel.push_back(sha256(left + right));
            }
            
            // Move up a level
            levels.push_back(nextLevel);
            currentLevel = nextLevel;
        }
        rootHash = currentLevel[0];
    }

    string getRoot() { return rootHash; }
};

// ==========================================
// Main Execution
// ==========================================
int main() {
    // 1. Load Data
    string filePath = "../Dataset/Musical_Instruments.json"; // Adjust if needed
    vector<Review> dataset;
    dataset.reserve(1600000); 

    cout << "Loading Dataset (IO phase)..." << endl;
    ifstream file(filePath);
    if (!file.is_open()) { cerr << "Error: File not found!" << endl; return 1; }

    string line;
    int count = 0;
    while (getline(file, line)) {
        try {
            json j = json::parse(line);
            Review r;
            r.id = j.value("asin", "UNK");
            string text = j.value("reviewText", "");
            double rating = j.value("overall", 0.0);
            r.rawData = r.id + to_string(rating) + text;
            dataset.push_back(r);
            count++;
        } catch (...) { continue; }
    }
    cout << "Dataset Loaded: " << dataset.size() << " records." << endl;
    cout << "------------------------------------------------" << endl;

    // 2. Measure ALGORITHMIC Performance
    cout << "Starting Basic Merkle Tree Construction..." << endl;

    auto start = high_resolution_clock::now();

    // --- ALGORITHM START ---
    BasicMerkleTree* tree = new BasicMerkleTree(dataset);
    // --- ALGORITHM END ---

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Construction Completed." << endl;
    cout << "Merkle Root: " << tree->getRoot() << endl;
    cout << "------------------------------------------------" << endl;
    cout << "BASELINE TIME (Serial): " << duration.count() << " ms" << endl;
    cout << "------------------------------------------------" << endl;

    return 0;
}