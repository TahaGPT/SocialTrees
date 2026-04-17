#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <thread>
#include <future>
#include <cstdlib>
#include <algorithm>

// Custom Include
#include "sha.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

// ==========================================
// 1. DATA STRUCTURE
// ==========================================
struct Review
{
    string id;
    string text;
    double rating;
    string rawData;

    void computeRawString()
    {
        rawData = id + to_string(rating) + text;
    }
};

// ==========================================
// 2. HELPER: Tabular Display
// ==========================================
void displayTable(const vector<Review> &data, int limit = 5)
{
    cout << "\n--- DATASET SAMPLE (Top " << limit << ") ---" << endl;
    cout << left << setw(20) << "Review ID"
         << setw(10) << "Rating"
         << "Review Text (Truncated)" << endl;
    cout << string(80, '-') << endl;

    for (int i = 0; i < limit && i < (int)data.size(); i++)
    {
        string shortText = data[i].text;
        if (shortText.length() > 50)
            shortText = shortText.substr(0, 47) + "...";

        cout << left << setw(20) << data[i].id
             << setw(10) << data[i].rating
             << shortText << endl;
    }
    cout << string(80, '-') << endl;
}

// ==========================================
// 3. CLASS: Parallel Merkle Tree
// ==========================================
class MerkleTree
{
private:
    vector<string> leaves;
    vector<vector<string>> levels;
    unordered_map<string, int> idToIndexMap;

public:
    string rootHash;

    MerkleTree(const vector<Review> &data)
    {
        auto start = high_resolution_clock::now();

        int totalRecords = data.size();
        leaves.resize(totalRecords);

        unsigned int numThreads = thread::hardware_concurrency();
        if (numThreads == 0)
            numThreads = 4;

        cout << "[System] Using " << numThreads << " Threads (Lock-Free Mode)..." << endl;

        vector<future<void>> futures;
        int chunkSize = totalRecords / numThreads;

        for (unsigned int t = 0; t < numThreads; t++)
        {
            int startIdx = t * chunkSize;
            int endIdx = (t == numThreads - 1) ? totalRecords : (t + 1) * chunkSize;

            futures.push_back(async(launch::async, [this, &data, startIdx, endIdx]()
                                    {
                for (int i = startIdx; i < endIdx; i++) {
                    leaves[i] = sha256(data[i].rawData);
                } }));
        }

        for (auto &f : futures)
            f.get();

        for (int i = 0; i < totalRecords; i++)
        {
            idToIndexMap[data[i].id] = i;
        }

        levels.push_back(leaves);
        buildTree();

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);

        // METRICS CALCULATION
        double avgTimePerRecord = (double)duration.count() / totalRecords;

        cout << "[Performance] Tree built in " << duration.count() << " ms." << endl;
        cout << "[Metrics] Avg Hash Time per Record: " << avgTimePerRecord << " ms" << endl;
    }

    void buildTree()
    {
        vector<string> currentLevel = leaves;
        while (currentLevel.size() > 1)
        {
            vector<string> nextLevel;
            nextLevel.reserve((currentLevel.size() + 1) / 2);

            for (size_t i = 0; i < currentLevel.size(); i += 2)
            {
                string left = currentLevel[i];
                string right = (i + 1 < currentLevel.size()) ? currentLevel[i + 1] : left;
                nextLevel.push_back(sha256(left + right));
            }
            levels.push_back(nextLevel);
            currentLevel = nextLevel;
        }
        rootHash = currentLevel[0];
    }

    string getRoot() { return rootHash; }

    tuple<bool, vector<string>, int> getProof(string reviewID)
    {
        vector<string> proof;
        if (idToIndexMap.find(reviewID) == idToIndexMap.end())
            return make_tuple(false, proof, -1);

        int index = idToIndexMap[reviewID];
        int originalIndex = index;

        for (size_t i = 0; i < levels.size() - 1; i++)
        {
            vector<string> &level = levels[i];

            if ((size_t)index % 2 == 1)
            {
                proof.push_back(level[index - 1]);
            }
            else
            {
                if ((size_t)index + 1 < level.size())
                    proof.push_back(level[index + 1]);
                else
                    proof.push_back(level[index]);
            }
            index /= 2;
        }
        return make_tuple(true, proof, originalIndex);
    }

    static bool verifyProof(string dataRaw, const vector<string> &proof, int index, string expectedRoot)
    {
        string currentHash = sha256(dataRaw);
        for (const string &siblingHash : proof)
        {
            if (index % 2 == 1)
                currentHash = sha256(siblingHash + currentHash);
            else
                currentHash = sha256(currentHash + siblingHash);
            index /= 2;
        }
        return currentHash == expectedRoot;
    }
};

// ==========================================
// 4. MAIN SYSTEM (CLI)
// ==========================================
int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    string filePath = "../Dataset/Musical_Instruments.json";
    vector<Review> dataset;
    string storedRootHash = "";

    dataset.reserve(1600000);

    cout << "===================================================" << endl;
    cout << "   Merkle Tree Integrity System (1.5M Records)     " << endl;
    cout << "===================================================" << endl;

    cout << "Loading Dataset..." << endl;
    ifstream file(filePath);
    if (!file.is_open())
    {
        cerr << "Error: File not found!" << endl;
        return 1;
    }

    string line;
    int count = 0;
    while (getline(file, line))
    {
        if (line.empty())
            continue;
        try
        {
            json j = json::parse(line);
            Review r;
            if (j.contains("asin"))
                r.id = j["asin"].get<string>() + "_" + to_string(count);
            else
                r.id = "UNKNOWN_" + to_string(count);

            r.text = j.contains("reviewText") ? j["reviewText"].get<string>() : "";
            r.rating = j.contains("overall") ? j["overall"].get<double>() : 0.0;

            r.computeRawString();
            dataset.push_back(r);
            count++;
        }
        catch (...)
        {
            continue;
        }
    }
    cout << "Total Records Loaded: " << dataset.size() << endl;

    // Show table immediately after load
    displayTable(dataset);

    cout << "\nConstructing Merkle Tree..." << endl;
    MerkleTree *tree = new MerkleTree(dataset);
    storedRootHash = tree->getRoot();
    cout << "Merkle Root: " << storedRootHash << endl;

    int choice;
    while (true)
    {
        cout << "\n---------------- MENU ----------------" << endl;
        cout << "1. Verify Integrity (Root Comparison)" << endl;
        cout << "2. Generate & Verify Proof" << endl;
        cout << "3. Simulate: MODIFY Record" << endl;
        cout << "4. Simulate: DELETE Record" << endl;
        cout << "5. Simulate: INSERT Fake Record" << endl;
        cout << "6. Run Experiment C (Benchmark)" << endl;
        cout << "7. Show Dataset Sample" << endl;
        cout << "8. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;

        if (choice == 1)
        {
            if (tree->getRoot() == storedRootHash)
                cout << "[SUCCESS] Integrity Verified.\n";
            else
                cout << "[ALERT] DATA TAMPERING DETECTED! Root Mismatch.\n";
        }
        else if (choice == 2)
        {
            string id;
            cout << "Enter ID (Try: " << dataset[dataset.size() / 2].id << "): ";
            cin >> id;

            auto start = high_resolution_clock::now();
            auto result = tree->getProof(id);
            auto stop = high_resolution_clock::now();

            bool found = get<0>(result);
            vector<string> proofPath = get<1>(result);
            int leafIndex = get<2>(result);

            if (found)
            {
                string rawData = "";
                for (const auto &r : dataset)
                {
                    if (r.id == id)
                    {
                        rawData = r.rawData;
                        break;
                    }
                }
                bool valid = MerkleTree::verifyProof(rawData, proofPath, leafIndex, storedRootHash);

                cout << "[FOUND] Path length: " << proofPath.size() << endl;
                cout << "Time: " << duration_cast<microseconds>(stop - start).count() / 1000.0 << " ms" << endl;
                if (valid)
                    cout << "[VERIFIED] Cryptographic Proof Valid." << endl;
                else
                    cout << "[FAILED] Proof Invalid!" << endl;
            }
            else
            {
                cout << "[FAIL] ID Not found\n";
            }
        }
        else if (choice == 3)
        {
            cout << "Simulating MODIFICATION..." << endl;
            dataset[0].text = "HACKED";
            dataset[0].computeRawString();
            delete tree;
            tree = new MerkleTree(dataset);
            cout << "New Root: " << tree->getRoot() << endl;
            if (tree->getRoot() != storedRootHash)
                cout << ">> TAMPERING DETECTED <<" << endl;
        }
        else if (choice == 4)
        {
            cout << "Simulating DELETION..." << endl;
            dataset.pop_back();
            delete tree;
            tree = new MerkleTree(dataset);
            cout << "New Root: " << tree->getRoot() << endl;
            if (tree->getRoot() != storedRootHash)
                cout << ">> TAMPERING DETECTED <<" << endl;
        }
        else if (choice == 5)
        {
            cout << "Simulating INSERTION..." << endl;
            Review fake;
            fake.id = "FAKE";
            fake.text = "Hack";
            fake.rating = 5.0;
            fake.computeRawString();
            dataset.push_back(fake);
            delete tree;
            tree = new MerkleTree(dataset);
            cout << "New Root: " << tree->getRoot() << endl;
            if (tree->getRoot() != storedRootHash)
                cout << ">> TAMPERING DETECTED <<" << endl;
        }
        else if (choice == 6)
        {
            cout << "Running Benchmark (1000 proofs)...\n";
            double totalTime = 0;
            int samples = 1000;
            for (int i = 0; i < samples; i++)
            {
                int randIndex = rand() % dataset.size();
                auto start = high_resolution_clock::now();
                tree->getProof(dataset[randIndex].id);
                totalTime += duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
            }
            double avgMs = (totalTime / samples) / 1000000.0;
            cout << "Average Latency: " << avgMs << " ms (Target <100ms: " << (avgMs < 100 ? "PASS" : "FAIL") << ")" << endl;
        }
        else if (choice == 7)
        {
            displayTable(dataset);
        }
        else
            break;
    }

    return 0;
}