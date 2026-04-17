/*
    ALGORITHM 2: Linear Merkle Tree (Array-Based / Implicit Heap)
    Optimization: Cache Locality (Spatial Locality)
    Complexity: O(N) Time, but with significantly lower constants due to CPU Cache.
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

class LinearMerkleTree {
private:
    vector<string> tree; // ONE single flat array
    int leafCount;
    int treeSize;
    int leafStartIndex;

public:
    LinearMerkleTree(const vector<Review>& data) {
        leafCount = data.size();
        
        // 1. Calculate Tree Size
        // In a Linear Heap, we usually pad to the next Power of 2 to make math easy (2*i)
        // Size of a complete binary tree with N leaves is roughly 2*N
        int height = ceil(log2(leafCount));
        int maxLeaves = pow(2, height);
        treeSize = 2 * maxLeaves; 
        leafStartIndex = maxLeaves;

        // Reserve memory in ONE block (Critical for Cache Locality)
        tree.resize(treeSize, ""); 

        // 2. Place Leaves at the end of the array
        for(int i = 0; i < leafCount; i++) {
            // Index logic: Leaves start at index 'leafStartIndex'
            tree[leafStartIndex + i] = sha256(data[i].rawData);
        }
        // Fill remaining padding with empty hash if necessary
        for(int i = leafCount; i < maxLeaves; i++) {
            tree[leafStartIndex + i] = sha256(""); 
        }

        // 3. Build Tree (Hashing Upwards)
        buildTree();
    }

    void buildTree() {
        // We start from the last parent node and work backwards to 1
        // Parent of k is k/2.
        for (int i = leafStartIndex - 1; i > 0; i--) {
            string left = tree[2 * i];
            string right = tree[2 * i + 1];
            tree[i] = sha256(left + right);
        }
    }

    string getRoot() {
        // In Heap layout, Root is always at index 1
        return tree[1];
    }
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
            string text = j.value("reviewText", "");
            double rating = j.value("overall", 0.0);
            r.rawData = r.id + to_string(rating) + text;
            dataset.push_back(r);
        } catch (...) { continue; }
    }
    cout << "Dataset Loaded: " << dataset.size() << endl;

    cout << "Starting LINEAR Tree Construction..." << endl;
    auto start = high_resolution_clock::now();

    // --- ALGORITHM START ---
    LinearMerkleTree* tree = new LinearMerkleTree(dataset);
    // --- ALGORITHM END ---

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Construction Completed." << endl;
    cout << "Merkle Root: " << tree->getRoot() << endl;
    cout << "------------------------------------------------" << endl;
    cout << "LINEAR TIME (Array): " << duration.count() << " ms" << endl;
    cout << "------------------------------------------------" << endl;

    return 0;
}