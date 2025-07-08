#include <immintrin.h>
#include <cstdint>
#include <array>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

using std::array;  using std::uint32_t;

// T-table declarations
array<uint32_t, 256> T0e, T1e, T2e, T3e;

static const uint8_t SBOX[256] = {
  0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
  0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
  0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
  0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
  0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
  0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
  0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
  0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
  0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
  0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
  0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
  0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
  0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
  0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
  0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
  0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

uint32_t rotl(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

uint32_t tau(uint32_t a) {
    return (static_cast<uint32_t>(SBOX[(a >> 24) & 0xFF]) << 24) |
        (static_cast<uint32_t>(SBOX[(a >> 16) & 0xFF]) << 16) |
        (static_cast<uint32_t>(SBOX[(a >> 8) & 0xFF]) << 8) |
        static_cast<uint32_t>(SBOX[a & 0xFF]);
}

uint32_t L(uint32_t b) {
    return b ^ rotl(b, 2) ^ rotl(b, 10) ^ rotl(b, 18) ^ rotl(b, 24);
}

uint32_t Te(uint32_t x) {
    return L(tau(x));
}

void gen_tables() {
    for (int i = 0; i < 256; ++i) {
        uint32_t t = i;
        t = tau(t << 24);
        T0e[i] = L(t);
        T1e[i] = rotl(T0e[i], 8);
        T2e[i] = rotl(T0e[i], 16);
        T3e[i] = rotl(T0e[i], 24);
    }
}

array<uint32_t, 32> key_schedule(const uint8_t MK[16]) {
    array<uint32_t, 32> rk;
    uint32_t FK[4] = { 0xA3B1BAC6,0x56AA3350,0x677D9197,0xB27022DC };
    uint32_t CK[32] = {
        0x00070E15,0x1C232A31,0x383F464D,0x545B6269,
        0x70777E85,0x8C939AA1,0xA8AFB6BD,0xC4CBD2D9,
        0xE0E7EEF5,0xFC030A11,0x181F262D,0x343B4249,
        0x50575E65,0x6C737A81,0x888F969D,0xA4ABB2B9,
        0xC0C7CED5,0xDCE3EAF1,0xF8FF060D,0x141B2229,
        0x30373E45,0x4C535A61,0x686F767D,0x848B9299,
        0xA0A7AEB5,0xBCC3CAD1,0xD8DFE6ED,0xF4FB0209,
        0x10171E25,0x2C333A41,0x484F565D,0x646B7279
    };

    uint32_t K[36];
    for (int i = 0; i < 4; ++i) {
        K[i] = (MK[4 * i] << 24) | (MK[4 * i + 1] << 16) | (MK[4 * i + 2] << 8) | MK[4 * i + 3];
        K[i] ^= FK[i];
    }

    for (int i = 0; i < 32; ++i) {
        uint32_t tmp = K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ CK[i];
        tmp = tau(tmp);
        tmp = tmp ^ rotl(tmp, 13) ^ rotl(tmp, 23);
        K[i + 4] = K[i] ^ tmp;
        rk[i] = K[i + 4];
    }
    return rk;
}

__m256i gather32(const uint32_t* base, __m256i idx) {
    return _mm256_i32gather_epi32(reinterpret_cast<const int*>(base), idx, 4);
}

static const __m256i MASK_FF = _mm256_set1_epi32(0xFF);

__m256i Te_vec(__m256i x) {
    __m256i i0 = _mm256_and_si256(_mm256_srli_epi32(x, 24), MASK_FF);
    __m256i i1 = _mm256_and_si256(_mm256_srli_epi32(x, 16), MASK_FF);
    __m256i i2 = _mm256_and_si256(_mm256_srli_epi32(x, 8), MASK_FF);
    __m256i i3 = _mm256_and_si256(x, MASK_FF);

    __m256i v0 = gather32(T0e.data(), i0);
    __m256i v1 = gather32(T1e.data(), i1);
    __m256i v2 = gather32(T2e.data(), i2);
    __m256i v3 = gather32(T3e.data(), i3);

    return _mm256_xor_si256(_mm256_xor_si256(v0, v1), _mm256_xor_si256(v2, v3));
}

void sm4_encrypt(const uint8_t in[8][16], uint8_t out[8][16], const array<uint32_t, 32>& rk) {
    __m256i X0, X1, X2, X3;
    uint32_t tmp[8];

    for (int b = 0; b < 8; ++b)
        tmp[b] = (in[b][0] << 24) | (in[b][1] << 16) | (in[b][2] << 8) | in[b][3];
    X0 = _mm256_loadu_si256((__m256i*)tmp);

    for (int b = 0; b < 8; ++b)
        tmp[b] = (in[b][4] << 24) | (in[b][5] << 16) | (in[b][6] << 8) | in[b][7];
    X1 = _mm256_loadu_si256((__m256i*)tmp);

    for (int b = 0; b < 8; ++b)
        tmp[b] = (in[b][8] << 24) | (in[b][9] << 16) | (in[b][10] << 8) | in[b][11];
    X2 = _mm256_loadu_si256((__m256i*)tmp);

    for (int b = 0; b < 8; ++b)
        tmp[b] = (in[b][12] << 24) | (in[b][13] << 16) | (in[b][14] << 8) | in[b][15];
    X3 = _mm256_loadu_si256((__m256i*)tmp);

    for (int r = 0; r < 32; ++r) {
        __m256i rk_vec = _mm256_set1_epi32(rk[r]);
        __m256i tmp = _mm256_xor_si256(_mm256_xor_si256(X1, X2), _mm256_xor_si256(X3, rk_vec));
        __m256i Xn = _mm256_xor_si256(X0, Te_vec(tmp));
        X0 = X1; X1 = X2; X2 = X3; X3 = Xn;
    }

    _mm256_storeu_si256((__m256i*)tmp, X3);
    for (int b = 0; b < 8; ++b) {
        uint32_t y = tmp[b];
        out[b][0] = y >> 24; out[b][1] = y >> 16; out[b][2] = y >> 8; out[b][3] = y;
    }
    _mm256_storeu_si256((__m256i*)tmp, X2);
    for (int b = 0; b < 8; ++b) {
        uint32_t y = tmp[b];
        out[b][4] = y >> 24; out[b][5] = y >> 16; out[b][6] = y >> 8; out[b][7] = y;
    }
    _mm256_storeu_si256((__m256i*)tmp, X1);
    for (int b = 0; b < 8; ++b) {
        uint32_t y = tmp[b];
        out[b][8] = y >> 24; out[b][9] = y >> 16; out[b][10] = y >> 8; out[b][11] = y;
    }
    _mm256_storeu_si256((__m256i*)tmp, X0);
    for (int b = 0; b < 8; ++b) {
        uint32_t y = tmp[b];
        out[b][12] = y >> 24; out[b][13] = y >> 16; out[b][14] = y >> 8; out[b][15] = y;
    }
}

void sm4_decrypt(const uint8_t in[8][16],
    uint8_t       out[8][16],
    const array<uint32_t, 32>& rk)
{
    array<uint32_t, 32> rkr;
    for (int i = 0; i < 32; ++i) rkr[i] = rk[31 - i];
    sm4_encrypt(in, out, rkr);
}

//int main() {
//    gen_tables();
//
//    uint8_t key[16] = {
//        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
//        0x38,0x39,0x61,0x62,0x63,0x64,0x65,0x66
//    };
//    uint8_t pt[16] = { 'h','e','l','l','o',',',' ','s','m','4',' ','d','e','m','o','!' };
//
//    auto rk = key_schedule(key);
//
//    // 复制同一明文到8块
//    uint8_t plain8[8][16];
//    uint8_t cipher8[8][16];
//    uint8_t back8[8][16];
//    for (int i = 0; i < 8; ++i) {
//        memcpy(plain8[i], pt, 16);
//    }
//
//    // 加密解密
//    sm4_encrypt(plain8, cipher8, rk);
//    sm4_decrypt(cipher8, back8, rk);
//
//    // 打印第0块结果示范
//    std::cout << "Plain : ";
//    for (auto b : plain8[0]) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
//    std::cout << "\nCipher: ";
//    for (auto b : cipher8[0]) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
//    std::cout << "\nDec   : ";
//    for (auto b : back8[0]) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
//    std::cout << '\n';
//
//    constexpr int N = 10000; 
//    int batch_size = 8;
//
//    auto t0 = std::chrono::high_resolution_clock::now();
//    for (int i = 0; i < N; ++i) sm4_encrypt(plain8, cipher8, rk);
//    auto t1 = std::chrono::high_resolution_clock::now();
//
//
////为了方便对比，我们时间都取加解密10000
//    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
//    double avg_ms_per_block = total_ms / batch_size;
//    std::cout << "SIMD encrypt time per block: " << avg_ms_per_block << " ms/block\n";
//
//    auto t2 = std::chrono::high_resolution_clock::now();
//    for (int i = 0; i < N; ++i) sm4_decrypt(cipher8, back8, rk);
//    auto t3 = std::chrono::high_resolution_clock::now();
//
//    double total_dec_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
//    double avg_dec_ms_per_block = total_dec_ms / batch_size;
//    std::cout << "SIMD decrypt time per block: " << avg_dec_ms_per_block << " ms/block\n";
//
//    return 0;
//}

// 线程执行的加密函数（对多个8块批次加密）
void thread_encrypt(const uint8_t* in, uint8_t* out, const array<uint32_t, 32>& rk, int batches) {
    for (int i = 0; i < batches; ++i) {
        sm4_encrypt(reinterpret_cast<const uint8_t(*)[16]>(in + i * 8 * 16),
            reinterpret_cast<uint8_t(*)[16]>(out + i * 8 * 16),
            rk);
    }
}

// 线程执行的解密函数（对多个8块批次解密）
void thread_decrypt(const uint8_t* in, uint8_t* out, const array<uint32_t, 32>& rk, int batches) {
    for (int i = 0; i < batches; ++i) {
        sm4_decrypt(reinterpret_cast<const uint8_t(*)[16]>(in + i * 8 * 16),
            reinterpret_cast<uint8_t(*)[16]>(out + i * 8 * 16),
            rk);
    }
}

int main() {
    gen_tables();

    uint8_t key[16] = {
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
        0x38,0x39,0x61,0x62,0x63,0x64,0x65,0x66
    };
    auto rk = key_schedule(key);

    constexpr int total_blocks = 8 * 10000; // 总块数，比如 10000 批次 * 8块/批次
    constexpr int batch_size = 8;            // SIMD 每批8块
    constexpr int total_batches = total_blocks / batch_size;

    // 多线程数量（根据CPU核心数设置）
    int thread_num = std::thread::hardware_concurrency();
    if (thread_num == 0) thread_num = 4;  // 若检测不到，默认4线程

    // 计算每线程分配多少批次
    int batches_per_thread = total_batches / thread_num;
    int leftover = total_batches % thread_num;

    // 分配输入输出缓冲区
    std::vector<uint8_t> plain(total_blocks * 16);
    std::vector<uint8_t> cipher(total_blocks * 16);
    std::vector<uint8_t> back(total_blocks * 16);

    // 初始化明文数据，简单用0x00填充，或根据需要自行填充
    for (int i = 0; i < total_blocks; ++i) {
        memcpy(&plain[i * 16], "hello, sm4 demo!", 16);
    }

    // 启动多线程加密
    std::vector<std::thread> enc_threads;
    int offset_batches = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < thread_num; ++i) {
        int current_batches = batches_per_thread + (i < leftover ? 1 : 0);
        enc_threads.emplace_back(thread_encrypt,
            plain.data() + offset_batches * batch_size * 16,
            cipher.data() + offset_batches * batch_size * 16,
            std::ref(rk),
            current_batches);
        offset_batches += current_batches;
    }
    for (auto& th : enc_threads) th.join();
    auto t1 = std::chrono::high_resolution_clock::now();

    double enc_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Multi-thread SIMD encrypt total time: " << enc_time_ms << " ms\n";
    std::cout << "Per block time: " << enc_time_ms / total_blocks << " ms/block\n";

    // 启动多线程解密
    std::vector<std::thread> dec_threads;
    offset_batches = 0;
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < thread_num; ++i) {
        int current_batches = batches_per_thread + (i < leftover ? 1 : 0);
        dec_threads.emplace_back(thread_decrypt,
            cipher.data() + offset_batches * batch_size * 16,
            back.data() + offset_batches * batch_size * 16,
            std::ref(rk),
            current_batches);
        offset_batches += current_batches;
    }
    for (auto& th : dec_threads) th.join();
    auto t3 = std::chrono::high_resolution_clock::now();

    double dec_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "Multi-thread SIMD decrypt total time: " << dec_time_ms << " ms\n";
    std::cout << "Per block time: " << dec_time_ms / total_blocks << " ms/block\n";

    // 简单校验：打印第一块加解密结果
    std::cout << "Plain[0]: ";
    for (int i = 0; i < 16; ++i) std::cout << std::hex << (int)plain[i] << ' ';
    std::cout << "\nCipher[0]: ";
    for (int i = 0; i < 16; ++i) std::cout << std::hex << (int)cipher[i] << ' ';
    std::cout << "\nBack[0] : ";
    for (int i = 0; i < 16; ++i) std::cout << std::hex << (int)back[i] << ' ';
    std::cout << "\n";

    return 0;
}