#include "sha.hpp"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdint> // <--- THIS WAS MISSING, IT FIXES THE ERROR

using namespace std;

class SHA256_Internal {
private:
    uint32_t state[8];
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    
    void transform(const uint8_t *data) {
        uint32_t m[64], a, b, c, d, e, f, g, h, t1, t2;
        for (uint32_t i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (uint32_t i = 16; i < 64; ++i)
            m[i] = (rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10)) + m[i - 7] + 
                   (rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3)) + m[i - 16];
        a = state[0]; b = state[1]; c = state[2]; d = state[3];
        e = state[4]; f = state[5]; g = state[6]; h = state[7];
        for (uint32_t i = 0; i < 64; ++i) {
            t1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + m[i];
            t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

public:
    SHA256_Internal() {
        datalen = 0; bitlen = 0;
        state[0] = 0x6a09e667; state[1] = 0xbb67ae85; state[2] = 0x3c6ef372; state[3] = 0xa54ff53a;
        state[4] = 0x510e527f; state[5] = 0x9b05688c; state[6] = 0x1f83d9ab; state[7] = 0x5be0cd19;
    }
    void update(const uint8_t *data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            this->data[datalen] = data[i];
            datalen++;
            if (datalen == 64) {
                transform(this->data);
                bitlen += 512;
                datalen = 0;
            }
        }
    }
    void update(const string &str) { update((const uint8_t*)str.c_str(), str.size()); }
    
    string final() {
        uint32_t i = datalen;
        if (datalen < 56) {
            data[i++] = 0x80;
            while (i < 56) data[i++] = 0x00;
        } else {
            data[i++] = 0x80;
            while (i < 64) data[i++] = 0x00;
            transform(data);
            memset(data, 0, 56);
        }
        bitlen += datalen * 8;
        data[63] = bitlen; data[62] = bitlen >> 8; data[61] = bitlen >> 16; data[60] = bitlen >> 24;
        data[59] = bitlen >> 32; data[58] = bitlen >> 40; data[57] = bitlen >> 48; data[56] = bitlen >> 56;
        transform(data);
        stringstream res;
        for(int j=0; j<8; j++) res << hex << setw(8) << setfill('0') << state[j];
        return res.str();
    }
};

// Wrapper Implementation
string sha256(string str) {
    SHA256_Internal checksum;
    checksum.update(str);
    return checksum.final();
}