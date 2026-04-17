/*
    ALGORITHM 4: Merkle Mountain Range (MMR)
    Logic: "Bagging" Peaks. Designed for Append-Only Logs (Blockchain).
    Complexity: O(N) to build, but O(log N) worst case for any append.
    Structure: A list of perfect binary trees.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono> 

#include "sha.hpp"
#include "json.hpp" 

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

struct Review {
    string id;          
    string rawData;     
};

// A "Peak" represents the root of a perfect sub-tree in the mountain range
struct MountainPeak {
    string hash;
    int height; // 0 = Leaf
};

class MMR {
private:
    vector<MountainPeak> peaks;

public:
    MMR() {}

    // The core MMR Logic: Append and "Fold"
    void append(string rawData) {
        // 1. Create a new leaf (Height 0)
        string currentHash = sha256(rawData);
        int currentHeight = 0;

        // 2. "Fold" the mountains
        // While the last peak in our list has the same height as our new peak,
        // we merge them to create a taller peak.
        while (!peaks.empty() && peaks.back().height == currentHeight) {
            string left = peaks.back().hash;
            string right = currentHash;
            
            // Hash them together
            currentHash = sha256(left + right);
            
            // Remove the left child (we merged it)
            peaks.pop_back();
            
            // Grow height
            currentHeight++;
        }

        // 3. Push the final peak
        peaks.push_back({currentHash, currentHeight});
    }

    // To get a single "Root" for the whole dataset, we bag all peaks from right to left
    string getBaggedRoot() {
        if (peaks.empty()) return "";
        
        string root = peaks.back().hash; // Start with right-most peak
        
        // Hash backwards
        for (int i = peaks.size() - 2; i >= 0; i--) {
            root = sha256(peaks[i].hash + root);
        }
        return root;
    }

    int getPeakCount() { return peaks.size(); }
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

    cout << "Starting MMR Construction (Append-Only Logic)..." << endl;
    
    auto start = high_resolution_clock::now();
    
    // --- ALGORITHM START ---
    MMR* mmr = new MMR();
    
    // Simulate streaming data (Adding 1 by 1)
    for(const auto& r : dataset) {
        mmr->append(r.rawData);
    }
    
    // Finalize Root
    string finalRoot = mmr->getBaggedRoot();
    // --- ALGORITHM END ---

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Construction Completed." << endl;
    cout << "MMR Bagged Root: " << finalRoot << endl;
    cout << "Number of Mountain Peaks: " << mmr->getPeakCount() << endl;
    cout << "------------------------------------------------" << endl;
    cout << "MMR TIME: " << duration.count() << " ms" << endl;
    cout << "------------------------------------------------" << endl;

    return 0;
}