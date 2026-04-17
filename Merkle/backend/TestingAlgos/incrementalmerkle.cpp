/*
    ALGORITHM 3: Incremental (Dynamic) Merkle Tree
    Logic: Linear Memory Layout + Path Update (Dirty Hashing)
    Complexity: 
        - Build: O(N)
        - Update: O(log N) <--- THE KEY OPTIMIZATION
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

class IncrementalMerkleTree {
private:
    vector<string> tree;
    int leafCount;
    int treeSize;
    int leafStartIndex;

public:
    IncrementalMerkleTree(const vector<Review>& data) {
        leafCount = data.size();
        
        // Calculate size (Next Power of 2)
        int height = ceil(log2(leafCount));
        int maxLeaves = pow(2, height);
        treeSize = 2 * maxLeaves; 
        leafStartIndex = maxLeaves;

        tree.resize(treeSize, ""); 

        // Initial Fill
        for(int i = 0; i < leafCount; i++) {
            tree[leafStartIndex + i] = sha256(data[i].rawData);
        }
        // Fill padding
        for(int i = leafCount; i < maxLeaves; i++) {
            tree[leafStartIndex + i] = sha256(""); 
        }

        // Build entire tree once
        for (int i = leafStartIndex - 1; i > 0; i--) {
            tree[i] = sha256(tree[2 * i] + tree[2 * i + 1]);
        }
    }

    // THE OPTIMIZED ALGORITHM: Update only the path to root
    // Complexity: O(log N) - Approx 21 steps for 1.5M records
    void updateRecord(int dataIndex, string newRawData) {
        int treeIndex = leafStartIndex + dataIndex;
        
        // 1. Update the Leaf
        tree[treeIndex] = sha256(newRawData);

        // 2. Bubble up to Root (Divide by 2)
        for (int i = treeIndex / 2; i > 0; i /= 2) {
            string left = tree[2 * i];
            string right = tree[2 * i + 1];
            
            // Re-hash only this node
            tree[i] = sha256(left + right);
        }
    }

    string getRoot() { return tree[1]; }
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

    // 1. INITIAL BUILD
    cout << "Building Tree..." << endl;
    auto startBuild = high_resolution_clock::now();
    IncrementalMerkleTree* tree = new IncrementalMerkleTree(dataset);
    auto stopBuild = high_resolution_clock::now();
    cout << "Initial Root: " << tree->getRoot() << endl;
    cout << "Build Time: " << duration_cast<milliseconds>(stopBuild - startBuild).count() << " ms" << endl;

    // 2. THE TEST: UPDATE ONE RECORD
    cout << "\n--- TESTING INCREMENTAL UPDATE LOGIC ---" << endl;
    cout << "Modifying Record #500..." << endl;
    
    auto startUpdate = high_resolution_clock::now();
    
    // Perform the O(log N) update
    tree->updateRecord(500, "THIS_IS_NEW_HACKED_DATA");
    
    auto stopUpdate = high_resolution_clock::now();
    auto durationUpdate = duration_cast<microseconds>(stopUpdate - startUpdate);

    cout << "Updated Root: " << tree->getRoot() << endl;
    cout << "------------------------------------------------" << endl;
    cout << "INCREMENTAL UPDATE TIME: " << durationUpdate.count() << " microseconds" << endl;
    cout << "(= " << durationUpdate.count() / 1000.0 << " ms)" << endl;
    cout << "------------------------------------------------" << endl;

    return 0;
}