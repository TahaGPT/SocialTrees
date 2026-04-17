#include "merkle_api.hpp"
#include "merkle.cpp"  // Need full implementation
#include "config.h"
#include <fstream>
#include <sstream>

MerkleAPI::MerkleAPI() : tree(nullptr), dataset(nullptr), currentDataset("") {}

MerkleAPI::~MerkleAPI() {
    // Tree and dataset are managed externally
}

void MerkleAPI::setTree(MerkleTree* t, vector<Review>* ds, const string& name) {
    tree = t;
    dataset = ds;
    currentDataset = name;
}

// List available datasets
json MerkleAPI::listDatasets() {
    json response;
    vector<string> datasets = ListingDatasts(Config::DATASET_DIR);
    
    response["success"] = true;
    response["datasets"] = json::array();
    
    for (const auto& ds : datasets) {
        response["datasets"].push_back({
            {"name", ds},
            {"path", Config::DATASET_DIR + ds}
        });
    }
    
    return response;
}

// Build tree for dataset
json MerkleAPI::buildTree(const string& datasetName) {
    json response;
    
    try {
        string filePath = Config::DATASET_DIR + datasetName;
        
        // Load dataset
        auto startLoad = high_resolution_clock::now();
        vector<Review> newDataset = loadDatasetMmap(filePath);
        auto stopLoad = high_resolution_clock::now();
        
        if (newDataset.empty()) {
            response["success"] = false;
            response["error"] = "Dataset is empty or failed to load";
            return response;
        }
        
        // Update dataset pointer
        if (dataset != nullptr) {
            delete dataset;
        }
        dataset = new vector<Review>(newDataset);
        
        // Clean up old tree
        if (tree != nullptr) {
            delete tree;
            tree = nullptr;
        }
        
        // Build new tree
        currentDataset = datasetName;
        cout << "[API] Constructing Merkle Tree..." << endl;
        tree = new MerkleTree(*dataset);
        cout << "[API] Tree built successfully." << endl;
        
        response["success"] = true;
        response["rootHash"] = tree->gettingroot();
        response["buildTime"] = tree->buildTimeSec * 1000;
        response["peakMemory"] = tree->peakMemoryMB;
        response["recordCount"] = dataset->size();
        
    } catch (const exception& e) {
        cout << "[API] Error building tree: " << e.what() << endl;
        response["success"] = false;
        response["error"] = string("Build failed: ") + e.what();
    } catch (...) {
        cout << "[API] Unknown error building tree" << endl;
        response["success"] = false;
        response["error"] = "Unknown error occurred during tree build";
    }
    
    return response;
}

// Get tree structure (limited depth for performance)
json MerkleAPI::getTreeStructure(int maxDepth) {
    json response;
    
    if (tree == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    response["success"] = true;
    response["dataset"] = currentDataset;
    response["rootHash"] = tree->gettingroot();
    response["totalRecords"] = dataset->size();
    response["maxDepth"] = maxDepth;
    response["nodes"] = json::array();
    
    // Calculate tree parameters
    long long leafCapacity = 1;
    int dataSize = dataset->size();
    while (leafCapacity < dataSize) {
        leafCapacity *= 4;
    }
    
    int leafStartIdx = (leafCapacity - 1) / 3;
    int totalNodes = leafStartIdx + dataSize; // Internal nodes + leaf nodes
    
    // Export all nodes (internal + leaves)
    int nodesAtDepth = 1;
    int nodeIndex = 0;
    int currentDepth = 0;
    
    // Add all internal nodes
    while (nodeIndex < leafStartIdx) {
        int nodesInThisLevel = std::min(nodesAtDepth, leafStartIdx - nodeIndex);
        for (int i = 0; i < nodesInThisLevel; i++) {
            response["nodes"].push_back(nodeToJson(nodeIndex, currentDepth));
            nodeIndex++;
        }
        currentDepth++;
        nodesAtDepth *= 4;
    }
    
    // Add all leaf nodes (the actual data records)
    int leafDepth = currentDepth;
    for (int i = 0; i < dataSize; i++) {
        json leafNode = nodeToJson(leafStartIdx + i, leafDepth);
        // Add reviewID for leaf nodes
        leafNode["reviewId"] = (*dataset)[i].id;
        response["nodes"].push_back(leafNode);
    }
    
    // Update maxDepth to actual depth
    response["maxDepth"] = leafDepth;
    
    return response;
}

// Convert node to JSON
json MerkleAPI::nodeToJson(int index, int depth) {
    json node;
    
    node["index"] = index;
    node["depth"] = depth;
    
    // Determine if leaf based on tree structure
    // In a quad tree, leaves start at leafStartIdx (which isn't exposed yet, but we can infer or add getter)
    // For visualization, we just need the hash and ID
    
    if (tree != nullptr) {
        node["hash"] = tree->getHashAt(index);
    } else {
        node["hash"] = "";
    }
    
    return node;
}

// Get root hash
json MerkleAPI::getRootHash() {
    json response;
    
    if (tree == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    response["success"] = true;
    response["rootHash"] = tree->gettingroot();
    response["dataset"] = currentDataset;
    
    return response;
}

// Generate proof for record
json MerkleAPI::generateProof(const string& recordId) {
    json response;
    
    if (tree == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    
    // C++11 compatible tuple unpacking
    tuple<bool, vector<string>, int> proofResult = tree->geetingproof(recordId);
    bool found = get<0>(proofResult);
    vector<string> proof = get<1>(proofResult);
    int dataIndex = get<2>(proofResult);
    
    if (!found) {
        response["success"] = false;
        response["error"] = "Record ID not found";
        return response;
    }
    
    response["success"] = true;
    response["recordId"] = recordId;
    response["dataIndex"] = dataIndex;
    response["proof"] = json::array();
    
    for (const auto& hash : proof) {
        response["proof"].push_back(hash);
    }
    
    // Add full record data (including text for node details modal)
    if (dataIndex >= 0 && dataIndex < (int)dataset->size()) {
        response["record"] = {
            {"id", (*dataset)[dataIndex].id},
            {"rating", (*dataset)[dataIndex].rating},
            {"text", (*dataset)[dataIndex].txt},
            {"rawData", (*dataset)[dataIndex].RawData}
        };
    }
    
    return response;
}

// Generate and verify proof in one optimized call (no data transfer overhead)
json MerkleAPI::generateAndVerifyProof(const string& recordId) {
    json response;
    
    if (tree == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    auto startTime = chrono::high_resolution_clock::now();
    
    // Generate proof
    auto proofStartTime = chrono::high_resolution_clock::now();
    tuple<bool, vector<string>, int> proofResult = tree->geetingproof(recordId);
    bool found = get<0>(proofResult);
    vector<string> proof = get<1>(proofResult);
    int dataIndex = get<2>(proofResult);
    auto proofEndTime = chrono::high_resolution_clock::now();
    
    if (!found) {
        response["success"] = false;
        response["error"] = "Record ID not found";
        return response;
    }
    
    // Verify proof
    auto verifyStartTime = chrono::high_resolution_clock::now();
    string rawData = (*dataset)[dataIndex].RawData;
    string expectedRoot = tree->gettingroot();
    bool isValid = MerkleTree::verifyProof(rawData, proof, dataIndex, expectedRoot);
    auto verifyEndTime = chrono::high_resolution_clock::now();
    
    auto totalEndTime = chrono::high_resolution_clock::now();
    
    // Calculate timings in microseconds for precision
    double proofTime = chrono::duration<double, milli>(proofEndTime - proofStartTime).count();
    double verifyTime = chrono::duration<double, milli>(verifyEndTime - verifyStartTime).count();
    double totalTime = chrono::duration<double, milli>(totalEndTime - startTime).count();
    
    response["success"] = true;
    response["recordId"] = recordId;
    response["dataIndex"] = dataIndex;
    response["isValid"] = isValid;
    response["proof"] = json::array();
    
    for (const auto& hash : proof) {
        response["proof"].push_back(hash);
    }
    
    // Add minimal record metadata (no large text fields)
    response["record"] = {
        {"id", (*dataset)[dataIndex].id},
        {"rating", (*dataset)[dataIndex].rating}
    };
    
    // Performance timings
    response["timings"] = {
        {"proofGeneration", proofTime},
        {"verification", verifyTime},
        {"total", totalTime}
    };
    
    return response;
}

// Verify proof
json MerkleAPI::verifyProof(const json& proofData) {
    json response;
    
    try {
        string rawData = proofData["rawData"];
        int dataIndex = proofData["dataIndex"];
        string expectedRoot = proofData["expectedRoot"];
        
        vector<string> proof;
        for (const auto& hash : proofData["proof"]) {
            proof.push_back(hash);
        }
        
        bool isValid = MerkleTree::verifyProof(rawData, proof, dataIndex, expectedRoot);
        
        response["success"] = true;
        response["isValid"] = isValid;
        
    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = e.what();
    }
    
    return response;
}

// Get statistics
json MerkleAPI::getStats() {
    json response;
    
    if (tree == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    response["success"] = true;
    response["dataset"] = currentDataset;
    response["recordCount"] = dataset->size();
    response["rootHash"] = tree->gettingroot();
    response["buildTime"] = tree->buildTimeSec;
    response["peakMemory"] = tree->peakMemoryMB;
    response["currentMemory"] = GttingMemoryUsageMB();
    
    // Calculate throughput
    response["throughput"] = (tree->buildTimeSec > 0) ? (dataset->size() / tree->buildTimeSec) : 0;
    response["avgHashTime"] = (dataset->size() > 0) ? (tree->buildTimeSec * 1000.0 / dataset->size()) : 0;
    
    // Benchmark proof generation (sample 5 random records)
    if (dataset->size() > 0) {
        double totalProofTime = 0;
        int sampleSize = std::min(5, (int)dataset->size());
        
        for (int i = 0; i < sampleSize; i++) {
            int idx = (i * dataset->size()) / sampleSize;
            auto proofStart = chrono::high_resolution_clock::now();
            tree->geetingproof((*dataset)[idx].id);
            auto proofEnd = chrono::high_resolution_clock::now();
            totalProofTime += chrono::duration<double, milli>(proofEnd - proofStart).count();
        }
        
        response["avgProofLatency"] = totalProofTime / sampleSize;
    } else {
        response["avgProofLatency"] = 0;
    }
    
    return response;
}

// Get root history
json MerkleAPI::getRootHistory() {
    json response;
    response["success"] = true;
    response["history"] = json::array();
    
    ifstream file(Config::ROOT_HISTORY_FILE);
    if (!file.is_open()) {
        return response;
    }
    
    string line;
    json entry;
    
    while (getline(file, line)) {
        if (line.find("Timestamp: ") == 0) {
            if (!entry.empty()) {
                response["history"].push_back(entry);
            }
            entry = json::object();
            entry["timestamp"] = line.substr(11);
        } else if (line.find("dataset: ") == 0) {
            entry["dataset"] = line.substr(9);
        } else if (line.find("Records: ") == 0) {
            entry["records"] = stoi(line.substr(9));
        } else if (line.find("Root: ") == 0) {
            entry["root"] = line.substr(6);
        }
    }
    
    if (!entry.empty()) {
        response["history"].push_back(entry);
    }
    
    file.close();
    return response;
}

// Update a record
json MerkleAPI::updateRecord(const string& recordId, const json& newData) {
    json response;
    
    if (tree == nullptr || dataset == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    // Find record by ID
    int dataIndex = -1;
    for (size_t i = 0; i < dataset->size(); i++) {
        if ((*dataset)[i].id == recordId) {
            dataIndex = i;
            break;
        }
    }
    
    if (dataIndex == -1) {
        response["success"] = false;
        response["error"] = "Record ID not found";
        return response;
    }
    
    // Store old root
    string oldRoot = tree->gettingroot();
    
    // Update record
    if (newData.contains("text")) {
        (*dataset)[dataIndex].txt = newData["text"];
    }
    if (newData.contains("rating")) {
        (*dataset)[dataIndex].rating = newData["rating"];
    }
    
    (*dataset)[dataIndex].computingactualstr();
    
    // Incremental update
    auto start = high_resolution_clock::now();
    tree->updatingtheleaf(dataIndex, (*dataset)[dataIndex].RawData);
    auto stop = high_resolution_clock::now();
    
    string newRoot = tree->gettingroot();
    
    // Save to history
    SavingRootToFile(newRoot, "merkle_roots.txt", currentDataset, dataset->size());
    
    response["success"] = true;
    response["recordId"] = recordId;
    response["dataIndex"] = dataIndex;
    response["oldRoot"] = oldRoot;
    response["newRoot"] = newRoot;
    response["updateTime"] = duration_cast<microseconds>(stop - start).count();
    
    return response;
}

// Delete a record
json MerkleAPI::deleteRecord(const string& recordId) {
    json response;
    
    if (tree == nullptr || dataset == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    // Find record by ID
    int dataIndex = -1;
    for (size_t i = 0; i < dataset->size(); i++) {
        if ((*dataset)[i].id == recordId) {
            dataIndex = i;
            break;
        }
    }
    
    if (dataIndex == -1) {
        response["success"] = false;
        response["error"] = "Record ID not found";
        return response;
    }
    
    // Store old root
    string oldRoot = tree->gettingroot();
    
    // Delete record
    dataset->erase(dataset->begin() + dataIndex);
    
    // Rebuild tree
    delete tree;
    auto start = high_resolution_clock::now();
    tree = new MerkleTree(*dataset);
    auto stop = high_resolution_clock::now();
    
    string newRoot = tree->gettingroot();
    
    // Save to history
    SavingRootToFile(newRoot, "merkle_roots.txt", currentDataset, dataset->size());
    
    response["success"] = true;
    response["recordId"] = recordId;
    response["oldRoot"] = oldRoot;
    response["newRoot"] = newRoot;
    response["rebuildTime"] = duration_cast<milliseconds>(stop - start).count();
    response["recordCount"] = dataset->size();
    
    return response;
}

// Insert a new record
json MerkleAPI::insertRecord(const json& recordData) {
    json response;
    
    if (tree == nullptr || dataset == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    // Store old root
    string oldRoot = tree->gettingroot();
    
    // Create new record
    Review newRecord;
    newRecord.id = recordData.value("id", "NEW_" + to_string(dataset->size()));
    newRecord.txt = recordData.value("text", "");
    newRecord.rating = recordData.value("rating", 0.0);
    newRecord.computingactualstr();
    
    // Add to dataset
    dataset->push_back(newRecord);
    
    // Rebuild tree
    delete tree;
    auto start = high_resolution_clock::now();
    tree = new MerkleTree(*dataset);
    auto stop = high_resolution_clock::now();
    
    string newRoot = tree->gettingroot();
    
    // Save to history
    SavingRootToFile(newRoot, "merkle_roots.txt", currentDataset, dataset->size());
    
    response["success"] = true;
    response["recordId"] = newRecord.id;
    response["oldRoot"] = oldRoot;
    response["newRoot"] = newRoot;
    response["rebuildTime"] = duration_cast<milliseconds>(stop - start).count();
    response["recordCount"] = dataset->size();
    
    return response;
}

// Batch update records
json MerkleAPI::batchUpdate(const json& updates) {
    json response;
    
    if (tree == nullptr || dataset == nullptr) {
        response["success"] = false;
        response["error"] = "No tree loaded";
        return response;
    }
    
    // Store old root
    string oldRoot = tree->gettingroot();
    
    vector<pair<int, string>> batchUpdates;
    
    for (const auto& update : updates) {
        string recordId = update["recordId"];
        json newData = update["newData"];
        
        // Find record
        int dataIndex = -1;
        for (size_t i = 0; i < dataset->size(); i++) {
            if ((*dataset)[i].id == recordId) {
                dataIndex = i;
                break;
            }
        }
        
        if (dataIndex != -1) {
            // Update record
            if (newData.contains("text")) {
                (*dataset)[dataIndex].txt = newData["text"];
            }
            if (newData.contains("rating")) {
                (*dataset)[dataIndex].rating = newData["rating"];
            }
            
            (*dataset)[dataIndex].computingactualstr();
            batchUpdates.push_back({dataIndex, (*dataset)[dataIndex].RawData});
        }
    }
    
    // Batch update tree
    auto start = high_resolution_clock::now();
    tree->batchUpdatation(batchUpdates);
    auto stop = high_resolution_clock::now();
    
    string newRoot = tree->gettingroot();
    
    // Save to history
    SavingRootToFile(newRoot, "merkle_roots.txt", currentDataset, dataset->size());
    
    response["success"] = true;
    response["updatedCount"] = batchUpdates.size();
    response["oldRoot"] = oldRoot;
    response["newRoot"] = newRoot;
    response["updateTime"] = duration_cast<microseconds>(stop - start).count();
    
    return response;
}
