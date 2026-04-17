#ifndef MERKLE_API_HPP
#define MERKLE_API_HPP

#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

// Forward declarations
struct Review;  // Defined in merkle.cpp
class MerkleTree;

/**
 * API Server for Merkle Tree Operations
 * Exposes HTTP JSON endpoints for frontend communication
 */
class MerkleAPI {
private:
    MerkleTree* tree;
    vector<Review>* dataset;
    string currentDataset;
    
public:
    MerkleAPI();
    ~MerkleAPI();
    
    // Dataset operations
    json listDatasets();
    json buildTree(const string& datasetName);
    
    // Tree operations
    json getTreeStructure(int maxDepth = 7);
    json getRootHash();
    json getNodeByIndex(int index);
    
    // Proof operations
    json generateProof(const string& recordId);
    json verifyProof(const json& proofData);
    json generateAndVerifyProof(const string& recordId);  // Optimized combined operation
    
    // Update operations
    json updateRecord(const string& recordId, const json& newData);
    json deleteRecord(const string& recordId);
    json insertRecord(const json& recordData);
    json batchUpdate(const json& updates);
    
    // Statistics
    json getStats();
    json getRootHistory();
    
    // Helper methods
    void setTree(MerkleTree* t, vector<Review>* ds, const string& name);
    json nodeToJson(int index, int depth);
};

#endif // MERKLE_API_HPP
