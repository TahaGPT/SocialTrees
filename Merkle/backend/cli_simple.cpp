/**
 * Simple CLI Interface for Merkle Tree - Works with existing merkle.cpp
 * Provides interactive menu for all required CLI features
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

// Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

string currentDataset = "../Dataset/small_dataset.jsonl";
vector<pair<string, string>> savedRoots;

void printHeader() {
    cout << BOLD << CYAN << "\n";
    cout << "========================================\n";
    cout << "  MERKLE TREE CLI INTERFACE\n";
    cout << "  Quad-Tree Integrity Verification\n";
    cout << "========================================\n";
    cout << RESET << "\n";
}

void printMenu() {
    cout << BOLD << GREEN << "\nMain Menu:\n" << RESET;
    cout << "  " << BOLD << "1." << RESET << " Display Dataset (Tabular)\n";
    cout << "  " << BOLD << "2." << RESET << " Build Merkle Tree\n";
    cout << "  " << BOLD << "3." << RESET << " Save Current Root Hash\n";
    cout << "  " << BOLD << "4." << RESET << " Compare with Saved Roots\n";
    cout << "  " << BOLD << "5." << RESET << " Generate Proof Path (via API)\n";
    cout << "  " << BOLD << "6." << RESET << " Simulate Tampering\n";
    cout << "  " << BOLD << "7." << RESET << " Run Performance Tests\n";
    cout << "  " << BOLD << "8." << RESET << " Change Dataset\n";
    cout << "  " << BOLD << "0." << RESET << " Exit\n";
    cout << BOLD << YELLOW << "\nEnter choice: " << RESET;
}

void displayDatasetTabular(const string& datasetFile, int maxRows = 20) {
    cout << BOLD << BLUE << "\n=== Dataset Display ===" << RESET << "\n";
    
    string command = "head -" + to_string(maxRows) + " " + datasetFile;
    
    cout << BOLD;
    cout << left << setw(25) << "Review ID" 
         << setw(10) << "Rating" 
         << setw(60) << "Review Text (Truncated)" << "\n";
    cout << string(95, '-') << RESET << "\n";
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        cout << RED << "Error: Cannot open dataset file\n" << RESET;
        return;
    }
    
    char buffer[4096];
    int count = 0;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr && count < maxRows) {
        string line(buffer);
        
        // Simple JSON parsing (extract rating and text)
        size_t overallPos = line.find("\"overall\":");
        size_t textPos = line.find("\"reviewText\":");
        size_t asinPos = line.find("\"asin\":");
        
        if (overallPos != string::npos && textPos != string::npos) {
            string rating = "N/A";
            string text = "";
            string asin = "B001234";
            
            // Extract ASIN
            if (asinPos != string::npos) {
                size_t start = line.find("\"", asinPos + 8) + 1;
                size_t end = line.find("\"", start);
                if (start != string::npos && end != string::npos) {
                    asin = line.substr(start, end - start);
                    if (asin.length() > 10) asin = asin.substr(0, 10);
                }
            }
            
            // Extract rating
            size_t ratingStart = overallPos + 10;
            size_t ratingEnd = line.find_first_of(",}", ratingStart);
            if (ratingEnd != string::npos) {
                rating = line.substr(ratingStart, ratingEnd - ratingStart);
            }
            
            // Extract text
            size_t textStart = line.find("\"", textPos + 14) + 1;
            size_t textEnd = line.find("\"", textStart);
            if (textStart != string::npos && textEnd != string::npos) {
                text = line.substr(textStart, min((size_t)57, textEnd - textStart));
                if (textEnd - textStart > 57) text += "...";
            }
            
            string uniqueId = asin + "_" + to_string(count) + "_0";
            
            cout << left << setw(25) << uniqueId
                 << setw(10) << rating
                 << setw(60) << text << "\n";
            
            count++;
        }
    }
    
    pclose(pipe);
    
    cout << BOLD << GREEN << "\nDisplayed " << count << " records\n" << RESET;
    
    // Count total
    string countCmd = "wc -l < " + datasetFile;
    pipe = popen(countCmd.c_str(), "r");
    if (pipe) {
        char totalBuf[100];
        if (fgets(totalBuf, sizeof(totalBuf), pipe)) {
            cout << BOLD << "(Total in dataset: " << totalBuf << ")" << RESET;
        }
        pclose(pipe);
    }
}

void buildTree() {
    cout << BOLD << BLUE << "\n=== Building Merkle Tree ===" << RESET << "\n";
    cout << "Dataset: " << currentDataset << "\n";
    cout << "Building tree using merkle program...\n\n";
    
    string command = "cd ../backend && echo '1\n" + currentDataset + "\n' | ./merkle 2>&1 | tail -20";
    int result = system(command.c_str());
    
    if (result == 0) {
        cout << GREEN << "\n✓ Tree build command executed\n" << RESET;
    } else {
        cout << RED << "\n✗ Build failed - make sure ./merkle is compiled\n" << RESET;
        cout << YELLOW << "Compile with: cd backend && make\n" << RESET;
    }
}

void saveRootHash() {
    cout << BOLD << BLUE << "\n=== Save Root Hash ===" << RESET << "\n";
    
    // Try to read from root_history.txt
    ifstream histFile("../backend/root_history.txt");
    string lastRoot;
    
    if (histFile.is_open()) {
        string line;
        while (getline(histFile, line)) {
            if (line.find("Root:") != string::npos) {
                size_t pos = line.find("Root:") + 6;
                lastRoot = line.substr(pos);
                // Remove any whitespace
                lastRoot.erase(0, lastRoot.find_first_not_of(" \t\n\r"));
                lastRoot.erase(lastRoot.find_last_not_of(" \t\n\r") + 1);
            }
        }
        histFile.close();
    }
    
    if (lastRoot.empty()) {
        cout << RED << "✗ No root hash found. Build tree first.\n" << RESET;
        return;
    }
    
    cout << "Current Root: " << CYAN << lastRoot.substr(0, 32) << "..." << RESET << "\n";
    cout << "Enter label for this root: ";
    string label;
    getline(cin >> ws, label);
    
    savedRoots.push_back({label, lastRoot});
    
    cout << GREEN << "✓ Root saved as: " << BOLD << label << RESET << "\n";
    
    // Save to file
    ofstream file("../backend/saved_roots.txt", ios::app);
    if (file.is_open()) {
        file << label << "|" << lastRoot << "\n";
        file.close();
    }
}

void compareRoots() {
    cout << BOLD << BLUE << "\n=== Compare Root Hashes ===" << RESET << "\n";
    
    // Load saved roots
    if (savedRoots.empty()) {
        ifstream file("../backend/saved_roots.txt");
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                size_t pos = line.find('|');
                if (pos != string::npos) {
                    savedRoots.push_back({line.substr(0, pos), line.substr(pos + 1)});
                }
            }
            file.close();
        }
    }
    
    if (savedRoots.empty()) {
        cout << YELLOW << "No saved roots to compare\n" << RESET;
        return;
    }
    
    // Get current root
    ifstream histFile("../backend/root_history.txt");
    string currentRoot;
    
    if (histFile.is_open()) {
        string line;
        while (getline(histFile, line)) {
            if (line.find("Root:") != string::npos) {
                size_t pos = line.find("Root:") + 6;
                currentRoot = line.substr(pos);
                currentRoot.erase(0, currentRoot.find_first_not_of(" \t\n\r"));
                currentRoot.erase(currentRoot.find_last_not_of(" \t\n\r") + 1);
            }
        }
        histFile.close();
    }
    
    if (currentRoot.empty()) {
        cout << RED << "✗ No tree built yet\n" << RESET;
        return;
    }
    
    cout << "Current Root: " << CYAN << currentRoot.substr(0, 32) << "..." << RESET << "\n\n";
    cout << left << setw(30) << "Saved Root Label" << setw(20) << "Status" << "Hash Preview\n";
    cout << string(80, '-') << "\n";
    
    for (const auto& saved : savedRoots) {
        bool match = (saved.second == currentRoot);
        cout << left << setw(30) << saved.first
             << setw(20) << (match ? GREEN "✓ MATCH" : RED "✗ DIFFERENT")
             << RESET << saved.second.substr(0, 24) << "...\n";
    }
}

void generateProof() {
    cout << BOLD << BLUE << "\n=== Generate Proof Path ===" << RESET << "\n";
    cout << YELLOW << "Note: Proof generation requires API server\n" << RESET;
    cout << "1. Start API server: cd backend && ./api_server\n";
    cout << "2. Use curl or web interface for proof generation\n\n";
    
    cout << "Enter Record ID to test (e.g., B001234_0_0): ";
    string recordId;
    getline(cin >> ws, recordId);
    
    cout << "\nExample curl command:\n";
    cout << CYAN << "curl -X POST http://localhost:8080/api/proof/generate \\\n";
    cout << "  -H 'Content-Type: application/json' \\\n";
    cout << "  -d '{\"recordId\":\"" << recordId << "\"}'\n" << RESET;
}

void simulateTampering() {
    cout << BOLD << BLUE << "\n=== Simulate Tampering ===" << RESET << "\n";
    cout << YELLOW << "⚠ This will modify the dataset to test integrity detection\n" << RESET;
    
    // Get current root before tampering
    ifstream histFile("../backend/root_history.txt");
    string rootBefore;
    
    if (histFile.is_open()) {
        string line;
        while (getline(histFile, line)) {
            if (line.find("Root:") != string::npos) {
                size_t pos = line.find("Root:") + 6;
                rootBefore = line.substr(pos);
                rootBefore.erase(0, rootBefore.find_first_not_of(" \t\n\r"));
                rootBefore.erase(rootBefore.find_last_not_of(" \t\n\r") + 1);
            }
        }
        histFile.close();
    }
    
    cout << "\nStep 1: Current root hash captured\n";
    cout << "Before: " << CYAN << rootBefore.substr(0, 32) << "..." << RESET << "\n";
    
    cout << "\nStep 2: Modifying dataset (simulating tampering)...\n";
    string tamperedFile = currentDataset + ".tampered";
    string copyCmd = "cp " + currentDataset + " " + tamperedFile;
    system(copyCmd.c_str());
    
    // Modify first line
    string modifyCmd = "sed -i '1s/great/TAMPERED/' " + tamperedFile;
    system(modifyCmd.c_str());
    
    cout << GREEN << "✓ Dataset tampered (copy created)\n" << RESET;
    
    cout << "\nStep 3: Rebuild tree with tampered data...\n";
    string buildCmd = "cd ../backend && echo '1\n" + tamperedFile + "\n' | ./merkle > /dev/null 2>&1";
    system(buildCmd.c_str());
    
    // Get new root
    histFile.open("../backend/root_history.txt");
    string rootAfter;
    
    if (histFile.is_open()) {
        string line;
        while (getline(histFile, line)) {
            if (line.find("Root:") != string::npos) {
                size_t pos = line.find("Root:") + 6;
                rootAfter = line.substr(pos);
                rootAfter.erase(0, rootAfter.find_first_not_of(" \t\n\r"));
                rootAfter.erase(rootAfter.find_last_not_of(" \t\n\r") + 1);
            }
        }
        histFile.close();
    }
    
    cout << "\nRoot Hash Comparison:\n";
    cout << "Before: " << CYAN << rootBefore.substr(0, 32) << "..." << RESET << "\n";
    cout << "After:  " << MAGENTA << rootAfter.substr(0, 32) << "..." << RESET << "\n";
    
    if (rootBefore != rootAfter) {
        cout << RED << "\n✗ TAMPERING DETECTED! Root hash changed.\n" << RESET;
        cout << GREEN << "✓ Integrity verification working correctly\n" << RESET;
    } else {
        cout << YELLOW << "⚠ Hashes are the same (unexpected)\n" << RESET;
    }
    
    // Cleanup
    cout << "\nCleaning up tampered file...\n";
    unlink(tamperedFile.c_str());
}

void runPerformanceTests() {
    cout << BOLD << BLUE << "\n=== Performance Tests ===" << RESET << "\n";
    cout << "Running performance benchmark...\n\n";
    
    string command = "cd ../backend && echo '9\n' | ./merkle 2>&1 | tail -30";
    system(command.c_str());
    
    cout << GREEN << "\n✓ Performance test completed\n" << RESET;
}

void changeDataset() {
    cout << BOLD << BLUE << "\n=== Change Dataset ===" << RESET << "\n";
    cout << "Available datasets:\n";
    cout << "  1. small_dataset.jsonl (40 records - fast)\n";
    cout << "  2. Musical_Instruments.json (1.5M records - large)\n";
    cout << "  3. Custom path\n";
    cout << "\nEnter choice (1-3): ";
    
    int choice;
    cin >> choice;
    
    switch (choice) {
        case 1:
            currentDataset = "../Dataset/small_dataset.jsonl";
            break;
        case 2:
            currentDataset = "../Dataset/Musical_Instruments.json";
            break;
        case 3:
            cout << "Enter dataset path: ";
            getline(cin >> ws, currentDataset);
            break;
        default:
            cout << RED << "Invalid choice\n" << RESET;
            return;
    }
    
    cout << GREEN << "✓ Dataset changed to: " << currentDataset << RESET << "\n";
}

int main() {
    // Load saved roots
    ifstream rootsFile("../backend/saved_roots.txt");
    if (rootsFile.is_open()) {
        string line;
        while (getline(rootsFile, line)) {
            size_t pos = line.find('|');
            if (pos != string::npos) {
                savedRoots.push_back({line.substr(0, pos), line.substr(pos + 1)});
            }
        }
        rootsFile.close();
    }
    
    printHeader();
    cout << "Welcome to the Merkle Tree CLI Interface\n";
    cout << "Using dataset: " << CYAN << currentDataset << RESET << "\n";
    cout << YELLOW << "\nNote: Ensure ./merkle is compiled in backend/\n" << RESET;
    cout << "Build with: " << CYAN << "cd backend && make\n" << RESET;
    
    while (true) {
        printMenu();
        
        int choice;
        cin >> choice;
        
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << RED << "Invalid input. Please enter a number.\n" << RESET;
            continue;
        }
        
        switch (choice) {
            case 0:
                cout << BOLD << GREEN << "\nThank you for using Merkle Tree CLI!\n" << RESET;
                return 0;
                
            case 1:
                displayDatasetTabular(currentDataset);
                break;
                
            case 2:
                buildTree();
                break;
                
            case 3:
                saveRootHash();
                break;
                
            case 4:
                compareRoots();
                break;
                
            case 5:
                generateProof();
                break;
                
            case 6:
                simulateTampering();
                break;
                
            case 7:
                runPerformanceTests();
                break;
                
            case 8:
                changeDataset();
                break;
                
            default:
                cout << RED << "Invalid choice. Please try again.\n" << RESET;
        }
        
        cout << "\nPress Enter to continue...";
        cin.ignore(10000, '\n');
        cin.get();
    }
    
    return 0;
}
