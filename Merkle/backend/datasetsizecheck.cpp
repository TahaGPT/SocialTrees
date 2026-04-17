#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    string filePath = "Dataset/Musical_Instruments.json";
    ifstream file(filePath);

    if (!file.is_open()) {
        cerr << "Error: Could not open file at " << filePath << endl;
        return 1;
    }

    cout << "Reading Line-Delimited JSON..." << endl;

    string line;
    long long count = 0;

    // This loop reads the file one line at a time
    // It is very memory efficient
    while (getline(file, line)) {
        if (!line.empty()) {
            count++;
        }
    }

    cout << "-----------------------------------" << endl;
    cout << "Total Elements: " << count << endl;
    cout << "-----------------------------------" << endl;

    return 0;
}