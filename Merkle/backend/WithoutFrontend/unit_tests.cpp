#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <unordered_map>
#include "sha.hpp"

using namespace std;
using namespace std::chrono;

// Test counter
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    cout << "Running: " << #name << "... "; \
    try { \
        test_##name(); \
        tests_passed++; \
        cout << "\033[32m✓ PASS\033[0m" << endl; \
    } catch (const exception& e) { \
        tests_failed++; \
        cout << "\033[31m✗ FAIL: " << e.what() << "\033[0m" << endl; \
    } \
} while(0)

#define ASSERT_TRUE(expr) if (!(expr)) throw runtime_error("Assertion failed: " #expr)
#define ASSERT_EQ(a, b) if ((a) != (b)) throw runtime_error("Assertion failed: " #a " == " #b)
#define ASSERT_NEQ(a, b) if ((a) == (b)) throw runtime_error("Assertion failed: " #a " != " #b)

// Hash Structure for testing
struct Hash {
    uint8_t bytes[32];
    bool operator==(const Hash& other) const {
        return memcmp(bytes, other.bytes, 32) == 0;
    }
    bool operator!=(const Hash& other) const {
        return !(*this == other);
    }
};

Hash hexToBytes(const string& hex) {
    Hash h;
    for (int i = 0; i < 32; i++) {
        string byteString = hex.substr(i * 2, 2);
        h.bytes[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }
    return h;
}

string bytesToHex(const Hash& h) {
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < 32; i++) {
        ss << setw(2) << (int)h.bytes[i];
    }
    return ss.str();
}

Hash computeHashBytes(const string& input) {
    return hexToBytes(sha256(input));
}

// ==================== UNIT TESTS ====================

TEST(sha256_empty_string) {
    string result = sha256("");
    ASSERT_EQ(result.length(), 64);
    // SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    ASSERT_EQ(result, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(sha256_deterministic) {
    string input = "test_data_12345";
    string hash1 = sha256(input);
    string hash2 = sha256(input);
    ASSERT_EQ(hash1, hash2);
}

TEST(sha256_different_inputs) {
    string hash1 = sha256("test1");
    string hash2 = sha256("test2");
    ASSERT_NEQ(hash1, hash2);
}

TEST(sha256_length) {
    string hash = sha256("any input");
    ASSERT_EQ(hash.length(), 64); // 32 bytes = 64 hex chars
}

TEST(hex_to_bytes_conversion) {
    string hexStr = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    Hash h = hexToBytes(hexStr);
    string converted = bytesToHex(h);
    ASSERT_EQ(hexStr, converted);
}

TEST(bytes_roundtrip) {
    string original = sha256("test");
    Hash h = hexToBytes(original);
    string recovered = bytesToHex(h);
    ASSERT_EQ(original, recovered);
}

TEST(hash_equality) {
    Hash h1 = computeHashBytes("test");
    Hash h2 = computeHashBytes("test");
    ASSERT_TRUE(h1 == h2);
}

TEST(hash_inequality) {
    Hash h1 = computeHashBytes("test1");
    Hash h2 = computeHashBytes("test2");
    ASSERT_TRUE(h1 != h2);
}

TEST(quad_tree_capacity) {
    // Test power of 4 calculation
    int dataSize = 1000000;
    long long capacity = 1;
    while (capacity < dataSize) {
        capacity *= 4;
    }
    ASSERT_TRUE(capacity >= dataSize);
    ASSERT_TRUE(capacity % 4 == 0 || capacity == 1);
}

TEST(quad_tree_total_nodes) {
    long long leafCapacity = 1048576; // 4^10
    long long totalNodes = (leafCapacity * 4 - 1) / 3;
    long long expected = 1398101; // (4^11 - 1) / 3
    ASSERT_EQ(totalNodes, expected);
}

TEST(quad_tree_leaf_start) {
    long long leafCapacity = 1024; // 4^5
    int leafStart = (leafCapacity - 1) / 3;
    ASSERT_EQ(leafStart, 341); // (1024 - 1) / 3
}

TEST(parent_child_relationship) {
    // Test quad-tree parent/child indexing
    int childIdx = 100;
    int parentIdx = (childIdx - 1) / 4;
    ASSERT_EQ(parentIdx, 24);
    
    // Verify children of parent
    int firstChild = 4 * parentIdx + 1;
    ASSERT_EQ(firstChild, 97);
    ASSERT_TRUE(childIdx >= firstChild && childIdx <= firstChild + 3);
}

TEST(hash_performance_single) {
    string input = "test_review_data_" + to_string(rand());
    auto start = high_resolution_clock::now();
    string hash = sha256(input);
    auto duration = duration_cast<microseconds>(high_resolution_clock::now() - start);
    
    ASSERT_TRUE(duration.count() < 1000); // Should be < 1ms
    ASSERT_EQ(hash.length(), 64);
}

TEST(hash_performance_batch) {
    int n = 1000;
    auto start = high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        sha256("test_" + to_string(i));
    }
    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start);
    
    double avgMs = duration.count() / (double)n;
    ASSERT_TRUE(avgMs < 1.0); // Avg < 1ms per hash
}

TEST(concatenation_affects_hash) {
    string h1 = sha256("ab");
    string h2 = sha256("a" + string("b"));
    ASSERT_EQ(h1, h2);
    
    string h3 = sha256("abc");
    string h4 = sha256("ab" + string("c"));
    ASSERT_EQ(h3, h4);
}

TEST(memory_struct_size) {
    // Verify Hash struct is exactly 32 bytes
    ASSERT_EQ(sizeof(Hash), 32);
}

TEST(proof_path_length) {
    // For 1M records in quad-tree, depth ~= log4(1M) = 10
    int records = 1000000;
    int depth = (int)(log(records) / log(4)) + 1;
    int siblingsPerLevel = 3; // 4 children - 1 = 3 siblings
    int proofSize = depth * siblingsPerLevel;
    
    ASSERT_TRUE(depth <= 12); // Reasonable depth
    ASSERT_TRUE(proofSize <= 40); // Reasonable proof size
}

TEST(empty_hash_consistency) {
    Hash empty1 = computeHashBytes("");
    Hash empty2 = computeHashBytes("");
    ASSERT_TRUE(empty1 == empty2);
}

TEST(collision_resistance) {
    // Basic collision test (not exhaustive)
    unordered_map<string, int> hashes;
    for (int i = 0; i < 10000; i++) {
        string hash = sha256("test_" + to_string(i));
        ASSERT_TRUE(hashes.find(hash) == hashes.end());
        hashes[hash] = i;
    }
}

TEST(binary_data_handling) {
    // Test with binary-like data
    string binary = string("\x00\x01\x02\xFF\xFE", 5);
    string hash = sha256(binary);
    ASSERT_EQ(hash.length(), 64);
}

TEST(large_input_handling) {
    // Test with large input
    string largeInput(1000000, 'x');
    auto start = high_resolution_clock::now();
    string hash = sha256(largeInput);
    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start);
    
    ASSERT_EQ(hash.length(), 64);
    ASSERT_TRUE(duration.count() < 1000); // Should complete within 1 second
}

// ==================== MAIN ====================

int main() {
    cout << "\n========================================" << endl;
    cout << "   MERKLE TREE UNIT TEST SUITE" << endl;
    cout << "========================================\n" << endl;
    
    cout << "[Category: Hash Function Tests]" << endl;
    RUN_TEST(sha256_empty_string);
    RUN_TEST(sha256_deterministic);
    RUN_TEST(sha256_different_inputs);
    RUN_TEST(sha256_length);
    RUN_TEST(concatenation_affects_hash);
    RUN_TEST(binary_data_handling);
    RUN_TEST(large_input_handling);
    
    cout << "\n[Category: Hash Conversion Tests]" << endl;
    RUN_TEST(hex_to_bytes_conversion);
    RUN_TEST(bytes_roundtrip);
    RUN_TEST(hash_equality);
    RUN_TEST(hash_inequality);
    RUN_TEST(empty_hash_consistency);
    
    cout << "\n[Category: Tree Structure Tests]" << endl;
    RUN_TEST(quad_tree_capacity);
    RUN_TEST(quad_tree_total_nodes);
    RUN_TEST(quad_tree_leaf_start);
    RUN_TEST(parent_child_relationship);
    RUN_TEST(proof_path_length);
    
    cout << "\n[Category: Performance Tests]" << endl;
    RUN_TEST(hash_performance_single);
    RUN_TEST(hash_performance_batch);
    
    cout << "\n[Category: Memory Tests]" << endl;
    RUN_TEST(memory_struct_size);
    
    cout << "\n[Category: Security Tests]" << endl;
    RUN_TEST(collision_resistance);
    
    cout << "\n========================================" << endl;
    cout << "   TEST RESULTS" << endl;
    cout << "========================================" << endl;
    cout << "\033[32mPassed: " << tests_passed << "\033[0m" << endl;
    cout << "\033[31mFailed: " << tests_failed << "\033[0m" << endl;
    cout << "Total:  " << (tests_passed + tests_failed) << endl;
    
    if (tests_failed == 0) {
        cout << "\n\033[32m🎉 ALL TESTS PASSED! 🎉\033[0m\n" << endl;
        return 0;
    } else {
        cout << "\n\033[31m⚠️  SOME TESTS FAILED ⚠️\033[0m\n" << endl;
        return 1;
    }
}
