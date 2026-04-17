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
#include <cstring>
#include <sstream>
#include <cmath>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <dirent.h>

#include "sha.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

// ==========================================
// 0. MEMORY & UTILITY FUNCTIONS
// ==========================================

// Get current memory usage in MB
double GttingMemoryUsageMB()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __linux__
    // Linux reports in KB
    return usage.ru_maxrss / 1024.0;
#else
    // MacOS reports in bytes
    return usage.ru_maxrss / (1024.0 * 1024.0);
#endif
}

// Save root hash to file with timestamp
bool SavingRootToFile(const string &root, const string &filename, const string &datastname, size_t recordCount)
{
    ofstream file(filename, ios::app);
    if (!file.is_open())
        return false;

    auto now = system_clock::now();
    auto now_c = system_clock::to_time_t(now);

    file << "Timestamp: " << ctime(&now_c);
    file << "dataset: " << datastname << "\n";
    file << "Records: " << recordCount << "\n";
    file << "Root: " << root << "\n";
    file << "---\n";
    file.close();
    return true;
}

// Load most recent root from file for specific dataset
string LoadngRootFromFile(const string &filename, const string &datastname)
{
    ifstream file(filename);
    if (!file.is_open())
        return "";

    string line, lastRoot;
    bool matchingDataset = false;

    while (getline(file, line))
    {
        if (line.find("dataset: " + datastname) != string::npos)
        {
            matchingDataset = true;
        }
        else if (matchingDataset && line.find("Root: ") == 0)
        {
            lastRoot = line.substr(6);
            matchingDataset = false;
        }
    }
    file.close();
    return lastRoot;
}

// List available datasets in directory
vector<string> ListingDatasts(const string &dirPath)
{
    vector<string> datasets;
    DIR *dir = opendir(dirPath.c_str());
    if (dir == nullptr)
        return datasets;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        string name = entry->d_name;
        if (name.find(".json") != string::npos && name[0] != '.')
        {
            datasets.push_back(name);
        }
    }
    closedir(dir);
    return datasets;
}

// ==========================================
// INPUT VALIDATION HELPERS
// ==========================================

// Helper to get a safe integer within a specific range
int GetingIntInpt(const string &prompt, int mn, int max, int mxretries = 10)
{
    int val;
    int attempts = 0;

    while (attempts < mxretries)
    {
        cout << prompt;

        // Chcking  if cin is in a bad state before reading
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        if (cin >> val)
        {
            // Check if there's a decimal point following (float input)
            char nextChar = cin.peek();
            if (nextChar == '.')
            {
                cout << ">> [Error] Please enter a WHOLE number (no decimals).\n";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                attempts++;
                continue;
            }

            // Input was a number. Now check if it's in the valid range.
            if (val >= mn && val <= max)
            {
                // Success! Clear any remaining junk on the line
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return val;
            }
            else
            {
                cout << ">> [Error] Number must be between " << mn << " and " << max << ".\n";
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
        }
        else
        {
            cout << ">> [Error] Invalid input. Please enter a NUMBER.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        attempts++;
    }

    // Max retries exceeded - return to main menu
    cout << ">> [Error] Too many invalid attempts. Returning to main menu.\n";
    return -1; 
}

// Helper to get a string (handles spaces correctly)
string GettingStringtypeInput(const string &prompt, bool alwingempty = false)
{
    string val;

    while (true)
    {
        // Ensure cin is in a good state
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        cout << prompt;
        getline(cin, val);

        // Trim whitespace
        size_t strt = val.find_first_not_of(" \t\n\r");
        size_t end = val.find_last_not_of(" \t\n\r");

        if (strt == string::npos)
        {
            val = "";
        }
        else
        {
            val = val.substr(strt, end - strt + 1);
        }

        // Check if empty
        if (!alwingempty && val.empty())
        {
            cout << ">> [Error] Input cannot be empty. Please try again.\n";
            continue;
        }

        return val;
    }
}
// ==========================================
// 1. DATA STRUCTURES & TYPES
// ==========================================

// OPTIMIZATION: Store Hash as 32 raw bytes (Not 64-byte Hex String)
// Reduces Memory Usage by ~66% and improves Cache Locality
struct Hash
{
    uint8_t bytes[32];

    bool operator==(const Hash &other) const
    {
        return memcmp(bytes, other.bytes, 32) == 0;
    }
    bool operator!=(const Hash &other) const
    {
        return !(*this == other);
    }
};

struct Review
{
    string id;
    string txt;
    double rating;
    string RawData;

    void computingactualstr()
    {
        RawData = id + "|" + to_string(rating) + "|" + txt;
    }
};

// ==========================================
// 2. LOW-LEVEL HELPERS (No <algorithm>)
// ==========================================

// Convert Hex String (from sha256 function) to Raw Bytes
Hash hexToBytes(const string &hex)
{
    Hash h;
    for (int i = 0; i < 32; i++)
    {
        string byteString = hex.substr(i * 2, 2);
        h.bytes[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }
    return h;
}

// Convert Raw Bytes to Hex String (for display/comparison)
string bytesToHex(const Hash &h)
{
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; i++)
    {
        ss << setw(2) << (int)h.bytes[i];
    }
    return ss.str();
}

// Compute SHA256 and return Hash Struct directly
Hash ComputingHashBytes(const string &input)
{
    // sha256 returns hex string, we convert immediately to save RAM
    return hexToBytes(sha256(input));
}

// Helper: Tabular Display
void displyingThetable(const vector<Review> &data, int limit = 5)
{
    cout << "\n--- dataset SAMPLE (Top " << limit << ") ---" << endl;
    cout << left << setw(20) << "Review ID"
         << setw(10) << "Rating"
         << "Review txt (Truncated)" << endl;
    cout << string(80, '-') << endl;

    int mxcount = (int)data.size();
    if (limit < mxcount)
        mxcount = limit;

    for (int i = 0; i < mxcount; i++)
    {
        string shrttxt = data[i].txt;
        if (shrttxt.length() > 50)
            shrttxt = shrttxt.substr(0, 47) + "...";

        cout << left << setw(20) << data[i].id
             << setw(10) << data[i].rating
             << shrttxt << endl;
    }
    cout << string(80, '-') << endl;
}

// ==========================================
// 3. PARALLEL dataset LOADER
// ==========================================
// ==========================================
// 3. MMAP PARALLEL LOADER (Zero-Copy I/O)
// ==========================================
vector<Review> loadDatasetMmap(const string &filePath)
{
    cout << "Mapping File to Memory (MMAP)..." << endl;

    // 1. Open File
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        cerr << "[Error] Could not open file." << endl;
        exit(1);
    }

    // 2. Get File Size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        cerr << "[Error] Could not stat file." << endl;
        exit(1);
    }
    size_t fileSize = sb.st_size;

    // 3. Map to Memory
    char *fileData = (char *)mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fileData == MAP_FAILED)
    {
        cerr << "[Error] MMAP failed." << endl;
        exit(1);
    }

    // 4. Parallel Parsing
    unsigned int numThreads = thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;

    vector<future<vector<Review>>> futures;
    size_t chunkSize = fileSize / numThreads;

    for (unsigned int t = 0; t < numThreads; t++)
    {
        // Calculate rough boundaries
        size_t startOffset = t * chunkSize;
        size_t endOffset = (t == numThreads - 1) ? fileSize : (t + 1) * chunkSize;

        // Adjust startOffset: Advance to next newline (unless we are at 0)
        if (t > 0)
        {
            while (startOffset < fileSize && fileData[startOffset] != '\n')
            {
                startOffset++;
            }
            startOffset++; // Move past the newline
        }

        // Adjust endOffset: Advance to next newline
        if (t < numThreads - 1)
        {
            while (endOffset < fileSize && fileData[endOffset] != '\n')
            {
                endOffset++;
            }
        }

        futures.push_back(async(launch::async, [fileData, startOffset, endOffset, t]()
                                {
            vector<Review> chunk;
            chunk.reserve((endOffset - startOffset) / 100); // Estimate ~100 bytes per line

            size_t cursor = startOffset;
            int localCount = 0;

            // Safety bound (3e18) prevents overflow loops
            while (cursor < endOffset && cursor < (size_t)3e18) { 
                // Find end of current line
                char* lineEnd = (char*)memchr(fileData + cursor, '\n', endOffset - cursor);
                if (!lineEnd) lineEnd = (char*)(fileData + endOffset);

                size_t lineLen = lineEnd - (fileData + cursor);
                
                if (lineLen > 2) { // Ignore empty lines
                    try {
                        // Create string from memory pointer (Zero copy overhead until here)
                        string lineStr(fileData + cursor, lineLen);
                        
                        json j = json::parse(lineStr);
                        Review r;
                        
                        // NEW ID LOGIC: Uses Thread ID (t) + Local Counter
                        if(j.contains("asin")) r.id = j["asin"].get<string>() + "_" + to_string(t) + "_" + to_string(localCount);
                        else r.id = "UNK_" + to_string(t) + "_" + to_string(localCount);
                        
                        r.txt = j.contains("reviewText") ? j["reviewText"].get<string>() : "";
                        r.rating = j.contains("overall") ? j["overall"].get<double>() : 0.0;
                        
                        r.computingactualstr();
                        chunk.push_back(r);
                        localCount++;
                    } catch (...) { /* Skip malformed lines */ }
                }
                cursor += lineLen + 1; // Move past \n
            }
            return chunk; }));
    }

    // 5. Merge Results (Optimized with Move Iterators)
    vector<Review> finalDataset;
    finalDataset.reserve(1600000);

    for (auto &f : futures)
    {
        vector<Review> chunk = f.get();
        // Uses move_iterator to avoid deep copying strings
        finalDataset.insert(finalDataset.end(), make_move_iterator(chunk.begin()), make_move_iterator(chunk.end()));
    }

    // 6. Cleanup
    munmap(fileData, fileSize);
    close(fd);

    return finalDataset;
}
// ==========================================
// 4. CLASS: OPTIMIZED MERKLE TREE
//    (Linear Quad-Tree + Byte Arrays)
// ==========================================
class MerkleTree
{
private:
    vector<Hash> tree; // OPTIMIZATION: Stores 32-byte structs, not strings
    unordered_map<string, int> idToDataIndex;

    int leafStartIdx;
    int treeSize;
    int dataSize;

public:
    string rootHashHex; // Store root as hex string for easy comparison
    double peakMemoryMB;
    double buildTimeSec;

    MerkleTree(const vector<Review> &data)
    {
        auto strt = high_resolution_clock::now();
        double startMem = GttingMemoryUsageMB();

        dataSize = data.size();

        // 1. QUAD TREE MATH
        long long leafCapacity = 1;
        while (leafCapacity < dataSize)
        {
            leafCapacity *= 4;
        }

        long long totalNodes = (leafCapacity * 4 - 1) / 3;
        leafStartIdx = (leafCapacity - 1) / 3;
        treeSize = totalNodes;

        // Resize Flat Array (Allocates Contiguous RAM)
        tree.resize(treeSize); // Default constructs empty Hashes

        // 2. PARALLEL LEAF HASHING
        unsigned int numThreads = thread::hardware_concurrency();
        if (numThreads == 0)
            numThreads = 4;

        cout << "[System] Using " << numThreads << " Threads (Linear Quad-Tree + Byte Arrays)..." << endl;

        vector<future<void>> futures;
        int chunkSize = dataSize / numThreads;

        for (unsigned int t = 0; t < numThreads; t++)
        {
            int startIdx = t * chunkSize;
            int endIdx = (t == numThreads - 1) ? dataSize : (t + 1) * chunkSize;

            futures.push_back(async(launch::async, [this, &data, startIdx, endIdx]()
                                    {
                for (int i = startIdx; i < endIdx; i++) {
                    // Store directly as Bytes
                    tree[leafStartIdx + i] = ComputingHashBytes(data[i].RawData);
                } }));
        }

        // Fill padding with hash of empty string
        Hash emptyHash = ComputingHashBytes("");
        for (int i = dataSize; i < leafCapacity; i++)
        {
            tree[leafStartIdx + i] = emptyHash;
        }

        for (auto &f : futures)
            f.get();

        // 3. MAP GENERATION
        for (int i = 0; i < dataSize; i++)
        {
            idToDataIndex[data[i].id] = i;
        }

        // 4. BUILD UPPER LEVELS
        buildTree();

        auto stp = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stp - strt);

        double endMem = GttingMemoryUsageMB();
        peakMemoryMB = (endMem > startMem) ? (endMem - startMem) : endMem;
        buildTimeSec = duration.count() / 1000.0;

        double avgTimePerRecord = (double)duration.count() / dataSize;
        cout << "[Performance] Tree built in " << duration.count() << " ms (" << buildTimeSec << "s)" << endl;
        cout << "[Memory] Peak usage: " << peakMemoryMB << " MB" << endl;
        cout << "[Metrics] Avg Hash Time per Record: " << avgTimePerRecord << " ms" << endl;
    }

    void buildTree()
    {
        // Iterate backwards from the last internal node to Root (0)
        for (int i = leafStartIdx - 1; i >= 0; i--)
        {
            // We need to concatenate the BYTES of the 4 children
            // String construction from bytes: string((char*)ptr, length)
            string combined;
            combined.reserve(128); // 32 * 4

            for (int k = 1; k <= 4; k++)
            {
                // Cast raw bytes to char for string append
                combined.append(reinterpret_cast<const char *>(tree[4 * i + k].bytes), 32);
            }

            tree[i] = ComputingHashBytes(combined);
        }
        rootHashHex = bytesToHex(tree[0]);
    }

    // INCREMENTAL UPDATE (Single)
    void updatingtheleaf(int dataIndex, const string &newRawData)
    {
        int currentIndex = leafStartIdx + dataIndex;
        tree[currentIndex] = ComputingHashBytes(newRawData);

        while (currentIndex > 0)
        {
            int parentIndex = (currentIndex - 1) / 4;

            string combined;
            combined.reserve(128);
            for (int k = 1; k <= 4; k++)
            {
                combined.append(reinterpret_cast<const char *>(tree[4 * parentIndex + k].bytes), 32);
            }

            tree[parentIndex] = ComputingHashBytes(combined);
            currentIndex = parentIndex;
        }
        rootHashHex = bytesToHex(tree[0]);
    }

    // BATCH UPDATE (Optimized for multiple changes)
    void batchUpdatation(const vector<pair<int, string>> &updates)
    {
        if (updates.empty())
            return;

        // Step 1: Update all leaves
        for (const auto &update : updates)
        {
            tree[leafStartIdx + update.first] = ComputingHashBytes(update.second);
        }

        // Step 2: Collect all affected parent nodes (use set to avoid duplicates)
        unordered_map<int, bool> affectedNodes;
        for (const auto &update : updates)
        {
            int current = leafStartIdx + update.first;
            while (current > 0)
            {
                int parent = (current - 1) / 4;
                affectedNodes[parent] = true;
                current = parent;
            }
        }

        // Step 3: Rebuild affected nodes from bottom to top
        for (int i = leafStartIdx - 1; i >= 0; i--)
        {
            if (affectedNodes.find(i) != affectedNodes.end())
            {
                string combined;
                combined.reserve(128);
                for (int k = 1; k <= 4; k++)
                {
                    combined.append(reinterpret_cast<const char *>(tree[4 * i + k].bytes), 32);
                }
                tree[i] = ComputingHashBytes(combined);
            }
        }
        rootHashHex = bytesToHex(tree[0]);
    }

    string gettingroot() { return rootHashHex; }

    // PROOF GENERATION
    tuple<bool, vector<string>, int> geetingproof(string reviewID)
    {
        vector<string> proof;
        if (idToDataIndex.find(reviewID) == idToDataIndex.end())
            return make_tuple(false, proof, -1);

        int dataIdx = idToDataIndex[reviewID];
        int currentIdx = leafStartIdx + dataIdx;

        while (currentIdx > 0)
        {
            int parentIdx = (currentIdx - 1) / 4;
            int groupStart = 4 * parentIdx + 1;

            for (int i = 0; i < 4; i++)
            {
                int siblingIdx = groupStart + i;
                if (siblingIdx != currentIdx)
                {
                    // Convert bytes back to Hex for the proof output (Client expects string)
                    proof.push_back(bytesToHex(tree[siblingIdx]));
                }
            }
            currentIdx = parentIdx;
        }
        return make_tuple(true, proof, dataIdx);
    }

    // VERIFICATION
    static bool verifyProof(string dataRaw, const vector<string> &proof, int dataIndex, string expectedRoot)
    {
        // We work with bytes internally for calculation
        Hash currentHash = ComputingHashBytes(dataRaw);

        int proofIdx = 0;
        long long currentPos = dataIndex;

        while (proofIdx < (int)proof.size())
        {
            int positionInGroup = currentPos % 4;

            string combined;
            combined.reserve(128);

            for (int i = 0; i < 4; i++)
            {
                if (i == positionInGroup)
                {
                    combined.append(reinterpret_cast<const char *>(currentHash.bytes), 32);
                }
                else
                {
                    if (proofIdx < (int)proof.size())
                    {
                        // Proof comes as Hex String, convert to bytes for math
                        Hash sibling = hexToBytes(proof[proofIdx++]);
                        combined.append(reinterpret_cast<const char *>(sibling.bytes), 32);
                    }
                    else
                        return false;
                }
            }

            currentHash = ComputingHashBytes(combined);
            currentPos /= 4;
        }

        return bytesToHex(currentHash) == expectedRoot;
    }
};

// ==========================================
// 5. MAIN SYSTEM
// ==========================================
int main()
{

#ifdef _WIN32
    cerr << "This version uses MMAP and requires Linux / Unix / MacOS." << endl;
    return 1;
#endif

    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    string datasetDir = "../Dataset/";
    string RootHistoryFile = "merkle_roots.txt";
    string filePath;
    string datastname;
    string thestoredHashOfRoot = "";

    // dataset selection menu
    cout << "===================================================" << endl;
    cout << "   Merkle Tree Multi-Category System" << endl;
    cout << "===================================================" << endl;

    vector<string> datasets = ListingDatasts(datasetDir);
    if (datasets.empty())
    {
        cerr << "[Error] No datasets found in " << datasetDir << endl;
        return 1;
    }

    cout << "\nAvailable Datasets:" << endl;
    for (size_t i = 0; i < datasets.size(); i++)
    {
        cout << "  [" << (i + 1) << "] " << datasets[i] << endl;
    }

    int datastchoice = GetingIntInpt("\nSelect dataset (1-" + to_string(datasets.size()) + "): ", 1, datasets.size());
    if (datastchoice == -1)
    {
        cout << "Invalid selection. Exiting." << endl;
        return 1;
    }

    datastname = datasets[datastchoice - 1];
    filePath = datasetDir + datastname;

    cout << "\n===================================================" << endl;
    cout << "   dataset: " << datastname << endl;
    cout << "   Architecture: Quad-Tree + MMAP + Parallel" << endl;
    cout << "===================================================" << endl;

    // 1. Load Data
    auto startLoad = high_resolution_clock::now();
    vector<Review> dataset = loadDatasetMmap(filePath);
    auto stopLoad = high_resolution_clock::now();

    // Handl ing Empty dataset
    if (dataset.empty())
    {
        cerr << "[Error] dataset is empty or failed to load. Exiting." << endl;
        return 1;
    }

    cout << "Total Records Loaded: " << dataset.size() << endl;
    cout << "Load Time: " << duration_cast<milliseconds>(stopLoad - startLoad).count() << " ms" << endl;

    displyingThetable(dataset);

    cout << "\nConstructing Merkle Tree..." << endl;
    MerkleTree *tree = new MerkleTree(dataset);
    thestoredHashOfRoot = tree->gettingroot();

    // Load historical root if exists
    string historicalroot = LoadngRootFromFile(RootHistoryFile, datastname);

    cout << "\n[Root Hash]" << endl;
    cout << "  Current: " << thestoredHashOfRoot << endl;
    if (!historicalroot.empty())
    {
        cout << "  Historical: " << historicalroot << endl;
        if (historicalroot == thestoredHashOfRoot)
        {
            cout << "  Status: ✓ MATCHES HISTORICAL RECORD" << endl;
        }
        else
        {
            cout << "  Status: ✗ DIFFERENT FROM HISTORY (DATA CHANGED)" << endl;
        }
    }

    // Save current root
    if (SavingRootToFile(thestoredHashOfRoot, RootHistoryFile, datastname, dataset.size()))
    {
        cout << "  Saved to: " << RootHistoryFile << endl;
    }

    int choice;
    while (true)
    {
        cout << "\n---------------- MENU ----------------" << endl;
        cout << "1. Verify Integrity" << endl;
        cout << "2. Generate & Verify Proof" << endl;
        cout << "3. Modify Record (Incremental)" << endl;
        cout << "4. Batch Modify (Optimized)" << endl;
        cout << "5. Delete Record (Rebuild)" << endl;
        cout << "6. Insert Fake Record (Rebuild)" << endl;
        cout << "7. Benchmark (1000 Proofs)" << endl;
        cout << "8. Show Dataset Sample" << endl;
        cout << "9. View Memory & Performance Stats" << endl;
        cout << "10. View Root History" << endl;
        cout << "11. Exit" << endl;
        choice = GetingIntInpt("Enter choice (1-11): ", 1, 11);

        if (choice == 1)
        {
            if (tree->gettingroot() == thestoredHashOfRoot)
                cout << "[SUCCESS] Integrity Verified.\n";
            else
                cout << "[ALERT] DATA TAMPERING DETECTED! Root Mismatch.\n";
        }
        else if (choice == 2)
        {
            cout << "\n=== PROOF GENERATION ===" << endl;
            cout << "You need to enter a Review ID to generate proof." << endl;
            // Suggest ID
            string sggestion = dataset[dataset.size() / 2].id;
            cout << "Example ID from dataset: " << sggestion << endl;
            // SAFE STRING INPUT (do not allow empty)
            string id = GettingStringtypeInput("Enter Review ID: ", false);

            auto strt = high_resolution_clock::now();
            auto result = tree->geetingproof(id);
            auto stp = high_resolution_clock::now();

            bool found = get<0>(result);
            vector<string> proofpth = get<1>(result);
            int leafindex = get<2>(result);

            if (found)
            {
                string RawData = "";
                for (const auto &r : dataset)
                {
                    if (r.id == id)
                    {
                        RawData = r.RawData;
                        break;
                    }
                }
                bool valid = MerkleTree::verifyProof(RawData, proofpth, leafindex, thestoredHashOfRoot);
                cout << "[FOUND] Path length: " << proofpth.size() / 3 << " levels" << endl;
                cout << "Time: " << duration_cast<microseconds>(stp - strt).count() / 1000.0 << " ms" << endl;
                if (valid)
                    cout << "[VERIFIED] Cryptographic Proof valid." << endl;
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
            cout << "\n=== MODIFY SINGLE RECORD ===" << endl;
            cout << "Step 1: Enter the record index (0 to " << dataset.size() - 1 << ")" << endl;
            int rcordingindex = GetingIntInpt("Enter record index: ", 0, dataset.size() - 1);

            // Check if GetingIntInpt failed (returned -1)
            if (rcordingindex == -1)
            {
                continue; // Return to main menu
            }

            cout << "Current txt: " << dataset[rcordingindex].txt.substr(0, 50) << "..." << endl;
            cout << "\nStep 2: Enter new review text (or press Enter to use empty string)" << endl;
            // SAFE STRING INPUT (allow empty for testing)
            string nwtxt = GettingStringtypeInput("New review text: ", true);

            // Modify RAM Data
            dataset[rcordingindex].txt = nwtxt;
            dataset[rcordingindex].computingactualstr();

            cout << "Performng Incremental Update..." << endl;
            auto strt = high_resolution_clock::now();
            tree->updatingtheleaf(rcordingindex, dataset[rcordingindex].RawData);
            auto stp = high_resolution_clock::now();

            string newRoot = tree->gettingroot();
            cout << "New Root: " << newRoot << endl;
            cout << "Update Time: " << duration_cast<microseconds>(stp - strt).count() << " microseconds!" << endl;
            if (newRoot != thestoredHashOfRoot)
            {
                cout << ">> TAMPERING DETECTED <<" << endl;
                // Auto-save modified root to history
                if (SavingRootToFile(newRoot, RootHistoryFile, datastname, dataset.size()))
                {
                    cout << "[Auto-saved] Modified root saved to history." << endl;
                }
            }
        }
        else if (choice == 4)
        {
            cout << "\n=== BATCH MODIFY (OPTIMIZED) ===" << endl;
            cout << "This will modify multiple random records at once." << endl;
            cout << "Enter how many records to modify (1-100):" << endl;
            // BATCH MODIFY - Optimized for multiple changes
            int numofchanges = GetingIntInpt("Number of records: ", 1, 100);
            if (numofchanges == -1)
                continue;

            vector<pair<int, string>> BatchUpdateion;
            cout << "\n[Batch Mode] Modifying " << numofchanges << " random records..." << endl;

            for (int i = 0; i < numofchanges; i++)
            {
                int randomindx = rand() % dataset.size();
                dataset[randomindx].txt = "BATCH_MODIFIED_" + to_string(i);
                dataset[randomindx].computingactualstr();
                BatchUpdateion.push_back({randomindx, dataset[randomindx].RawData});
            }

            auto strt = high_resolution_clock::now();
            tree->batchUpdatation(BatchUpdateion);
            auto stp = high_resolution_clock::now();

            string newRoot = tree->gettingroot();
            cout << "New Root: " << newRoot << endl;
            cout << "Batch Update Time: " << duration_cast<microseconds>(stp - strt).count() << " μs" << endl;
            cout << "Avg per record: " << duration_cast<microseconds>(stp - strt).count() / (double)numofchanges << " μs" << endl;

            if (newRoot != thestoredHashOfRoot)
            {
                cout << ">> TAMPERING DETECTED <<" << endl;
                // Auto-save modified root to history
                if (SavingRootToFile(newRoot, RootHistoryFile, datastname, dataset.size()))
                {
                    cout << "[Auto-saved] Modified root saved to history." << endl;
                }
            }
        }
        else if (choice == 5)
        {
            cout << "\n=== DELETE RECORD ===" << endl;
            cout << "This will delete the last record and rebuild the tree." << endl;
            cout << "No input needed - press Enter to continue..." << endl;
            cin.ignore();
            cout << "Simulating DELETION (Last Record)..." << endl;
            if (!dataset.empty())
            {
                dataset.pop_back();
                delete tree;
                auto rebuildingstart = high_resolution_clock::now();
                tree = new MerkleTree(dataset);
                auto rebuildingtime = duration_cast<milliseconds>(high_resolution_clock::now() - rebuildingstart).count();
                cout << "Rebuild Time: " << rebuildingtime << " ms" << endl;
                string newRoot = tree->gettingroot();
                cout << "New Root: " << newRoot << endl;
                if (newRoot != thestoredHashOfRoot)
                {
                    cout << ">> TAMPERING DETECTED <<" << endl;
                    // Auto-save modified root to history
                    if (SavingRootToFile(newRoot, RootHistoryFile, datastname, dataset.size()))
                    {
                        cout << "[Auto-saved] Modified root saved to history." << endl;
                    }
                }
            }
        }
        else if (choice == 6)
        {
            cout << "\n=== INSERT FAKE RECORD ===" << endl;
            cout << "This will insert a fake review and rebuild the tree." << endl;
            cout << "No input needed - press Enter to continue..." << endl;
            cin.ignore();
            cout << "Simulating INSERTION..." << endl;
            Review fake;
            fake.id = "FAKE";
            fake.txt = "Hack";
            fake.rating = 5.0;
            fake.computingactualstr();
            dataset.push_back(fake);
            delete tree;
            auto rebuildingstart = high_resolution_clock::now();
            tree = new MerkleTree(dataset);
            auto rebuildingtime = duration_cast<milliseconds>(high_resolution_clock::now() - rebuildingstart).count();
            cout << "Rebuild Time: " << rebuildingtime << " ms" << endl;
            string newRoot = tree->gettingroot();
            cout << "New Root: " << newRoot << endl;
            if (newRoot != thestoredHashOfRoot)
            {
                cout << ">> TAMPERING DETECTED <<" << endl;
                // Auto-save modified root to history
                if (SavingRootToFile(newRoot, RootHistoryFile, datastname, dataset.size()))
                {
                    cout << "[Auto-saved] Modified root saved to history." << endl;
                }
            }
        }
        else if (choice == 7)
        {
            cout << "\n=== BENCHMARK TEST ===" << endl;
            cout << "This will generate proofs for 1000 random records." << endl;
            cout << "No input needed - starting benchmark..." << endl;
            cout << "Running Benchmark (1000 proofs)...\n";
            double TotTime = 0;
            int smples = 1000;
            for (int i = 0; i < smples; i++)
            {
                int randIndex = rand() % dataset.size();
                auto strt = high_resolution_clock::now();
                tree->geetingproof(dataset[randIndex].id);
                TotTime += duration_cast<nanoseconds>(high_resolution_clock::now() - strt).count();
            }
            double AvgMs = (TotTime / smples) / 1000000.0;
            cout << "Average Latency: " << AvgMs << " ms (Target <100ms: " << (AvgMs < 100 ? "PASS" : "FAIL") << ")" << endl;
        }
        else if (choice == 9)
        {
            // Memory and performance statistics
            cout << "\n========== PERFORMANCE STATISTICS ==========" << endl;
            cout << "dataset: " << datastname << endl;
            cout << "Total Records: " << dataset.size() << endl;
            cout << "\n[Memory Metrics]" << endl;
            cout << "  Peak Usage: " << tree->peakMemoryMB << " MB" << endl;
            cout << "  Current Usage: " << GttingMemoryUsageMB() << " MB" << endl;
            cout << "\n[Build Performance]" << endl;
            cout << "  Build Time: " << tree->buildTimeSec << " seconds" << endl;
            cout << "  Records/sec: " << (int)(dataset.size() / tree->buildTimeSec) << endl;
            cout << "\n[Tree Structure]" << endl;
            cout << "  Type: Quad-tree (4-ary)" << endl;
            cout << "  Estimated Depth: ~" << (int)(log(dataset.size()) / log(4)) << " levels" << endl;
            cout << "  Root Hash: " << tree->gettingroot().substr(0, 16) << "..." << endl;
            cout << "==========================================" << endl;
        }
        else if (choice == 10)
        {
            // View root history
            cout << "\n========== ROOT HASH HISTORY ==========" << endl;
            ifstream HistFile(RootHistoryFile);
            if (HistFile.is_open())
            {
                string line;
                int count = 0;
                cout << "\nShowing entries for: " << datastname << "\n"
                     << endl;
                bool ShowingEntry = false;
                while (getline(HistFile, line))
                {
                    if (line.find("dataset: " + datastname) != string::npos)
                    {
                        ShowingEntry = true;
                        count++;
                        cout << "\n[Entry #" << count << "]" << endl;
                    }
                    if (ShowingEntry)
                    {
                        cout << line << endl;
                        if (line == "---")
                            ShowingEntry = false;
                    }
                }
                HistFile.close();
                if (count == 0)
                {
                    cout << "No history found for this dataset." << endl;
                }
            }
            else
            {
                cout << "No history file found." << endl;
            }
            cout << "======================================" << endl;
        }
        else if (choice == 8)
        {
            cout << "\n=== DATASET SAMPLE ===" << endl;
            cout << "Choose what you want to see:" << endl;
            cout << "  [1] Show Top 5 Sample Records (no further input)" << endl;
            cout << "  [2] Inspect Specific Record (you'll enter an index)" << endl;
            // SAFE SUB-MENU INPUT
            int SubChoice = GetingIntInpt("Enter your choice (1 or 2): ", 1, 2);

            // Check if GetingIntInpt failed
            if (SubChoice == -1)
            {
                continue; // Return to main menu
            }

            if (SubChoice == 1)
            {
                displyingThetable(dataset);
            }


            else
            {
                cout << "\nEnter the record index you want to inspect." << endl;
                cout << "Valid range: 0 to " << dataset.size() - 1 << endl;
                int idx = GetingIntInpt("Record index: ", 0, dataset.size() - 1);

                // Check if GetingIntInpt failed
                if (idx == -1)
                {
                    continue; // Return to main menu
                }

                cout << "\n--- RECORD #" << idx << " DETAILS ---" << endl;
                cout << "ID:      " << dataset[idx].id << endl;
                cout << "Rating:  " << dataset[idx].rating << endl;
                cout << "txt:    " << dataset[idx].txt << endl;
                cout << "RawData: " << dataset[idx].RawData.substr(0, 50) << "..." << endl;
                cout << "---------------------------------" << endl;
            }
        }
        else
            break;
    }

    return 0;
}