#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <immintrin.h> 
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <tuple>  
using namespace std;
using namespace chrono;


//二进制转换为十六进制函数实现
string BinToHex(string str) {
	string hex = "";
	int temp = 0;
	while (str.size() % 4 != 0) {
		str = "0" + str;
	}
	for (int i = 0; i < str.size(); i += 4) {
		temp = (str[i] - '0') * 8 + (str[i + 1] - '0') * 4 + (str[i + 2] - '0') * 2 + (str[i + 3] - '0') * 1;
		if (temp < 10) {
			hex += to_string(temp);
		}
		else {
			hex += 'A' + (temp - 10);
		}
	}
	return hex;
}

//十六进制转换为二进制函数实现
string HexToBin(string str) {
	string bin = "";
	string table[16] = { "0000","0001","0010","0011","0100","0101","0110","0111","1000","1001","1010","1011","1100","1101","1110","1111" };
	for (int i = 0; i < str.size(); i++) {
		if (str[i] >= 'A' && str[i] <= 'F') {
			bin += table[str[i] - 'A' + 10];
		}
		else {
			bin += table[str[i] - '0'];
		}
	}
	return bin;
}

//二进制转换为十进制的函数实现
int BinToDec(string str) {
	int dec = 0;
	for (int i = 0; i < str.size(); i++) {
		dec += (str[i] - '0') * pow(2, str.size() - i - 1);
	}
	return dec;
}

//十进制转换为二进制的函数实现
string DecToBin(int str) {
	string bin = "";
	while (str >= 1) {
		bin = to_string(str % 2) + bin;
		str = str / 2;
	}
	return bin;
}

//十六进制转换为十进制的函数实现
int HexToDec(string str) {
	int dec = 0;
	for (int i = 0; i < str.size(); i++) {
		if (str[i] >= 'A' && str[i] <= 'F') {
			dec += (str[i] - 'A' + 10) * pow(16, str.size() - i - 1);
		}
		else {
			dec += (str[i] - '0') * pow(16, str.size() - i - 1);
		}
	}
	return dec;
}

//十进制转换为十六进制的函数实现
string DecToHex(int str) {
	string hex = "";
	int temp = 0;
	while (str >= 1) {
		temp = str % 16;
		if (temp < 10 && temp >= 0) {
			hex = to_string(temp) + hex;
		}
		else {
			hex += ('A' + (temp - 10));
		}
		str = str / 16;
	}
	return hex;
}

string padding(string str) {
	string res = "";
	for (char c : str) {
		char buf[3];
		sprintf_s(buf, "%02X", (unsigned char)c);  // 始终两位16进制
		res += buf;
	}
	// 后续填充保持不变
	int res_length = res.size() * 4;

	res += "8";
	while (res.size() % 128 != 112) {
		res += "0";
	}

	string res_len = DecToHex(res_length);
	while (res_len.size() < 16) {
		res_len = "0" + res_len;
	}
	res += res_len;

	if (res.size() % 128 != 0) {
		throw runtime_error("Padding error: result length is not multiple of 128.");
	}
	return res;
}



string LeftShift(string str, int len) {
	string res = HexToBin(str);
	res = res.substr(len) + res.substr(0, len);
	return BinToHex(res);
}

inline uint32_t XOR32(uint32_t a, uint32_t b) {
	return a ^ b;
}
inline uint32_t AND32(uint32_t a, uint32_t b) {
	return a & b;
}
inline uint32_t OR32(uint32_t a, uint32_t b) {
	return a | b;
}
inline uint32_t NOT32(uint32_t a) {
	return ~a;
}
inline uint32_t ROTL32(uint32_t x, int n) {
	return (x << n) | (x >> (32 - n));
}


char binXor(char str1, char str2) {
	return str1 == str2 ? '0' : '1';
}

char binAnd(char str1, char str2) {
	return (str1 == '1' && str2 == '1') ? '1' : '0';
}

string ModAdd(string str1, string str2) {//mod 2^32运算的函数实现
	string res1 = HexToBin(str1);
	string res2 = HexToBin(str2);
	char temp = '0';
	string res = "";
	for (int i = res1.size() - 1; i >= 0; i--) {
		res = binXor(binXor(res1[i], res2[i]), temp) + res;
		if (binAnd(res1[i], res2[i]) == '1') {
			temp = '1';
		}
		else {
			if (binXor(res1[i], res2[i]) == '1') {
				temp = binAnd('1', temp);
			}
			else {
				temp = '0';
			}
		}
	}
	return BinToHex(res);
}

inline uint32_t P1(uint32_t X) {
	return X ^ ROTL32(X, 15) ^ ROTL32(X, 23);
}


inline uint32_t P0(uint32_t X) {
	return X ^ ROTL32(X, 9) ^ ROTL32(X, 17);
}

inline uint32_t FF(uint32_t X, uint32_t Y, uint32_t Z, int j) {
	return (j <= 15) ? (X ^ Y ^ Z) : ((X & Y) | (X & Z) | (Y & Z));
}

inline uint32_t GG(uint32_t X, uint32_t Y, uint32_t Z, int j) {
	return (j <= 15) ? (X ^ Y ^ Z) : ((X & Y) | ((~X) & Z));
}

inline uint32_t T(int j) {
	return (j <= 15) ? 0x79CC4519 : 0x7A879D8A;
}


void extension_SIMD(const uint32_t B[16], uint32_t W[68], uint32_t W1[64]) {
	// W[0..15] 直接赋值
	for (int i = 0; i < 16; ++i)
		W[i] = B[i];

	// W[16..67]：可以优化，但暂用普通逻辑
	for (int j = 16; j < 68; ++j) {
		uint32_t tmp = W[j - 16] ^ W[j - 9] ^ ROTL32(W[j - 3], 15);
		W[j] = P1(tmp) ^ ROTL32(W[j - 13], 7) ^ W[j - 6];
	}

	// SIMD优化 W′[0..63] = W[i] ⊕ W[i+4]
	for (int i = 0; i < 64; i += 8) {
		__m256i v1 = _mm256_loadu_si256((__m256i*)(W + i));
		__m256i v2 = _mm256_loadu_si256((__m256i*)(W + i + 4));
		__m256i v3 = _mm256_xor_si256(v1, v2);
		_mm256_storeu_si256((__m256i*)(W1 + i), v3);
	}
}


void compress(uint32_t V[8], const uint32_t B[16]) {
	uint32_t W[68], W1[64];
	extension_SIMD(B, W, W1);

	uint32_t A = V[0], B_ = V[1], C = V[2], D = V[3];
	uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

	for (int j = 0; j < 64; j++) {
		uint32_t SS1 = ROTL32((ROTL32(A, 12) + E + ROTL32(T(j), j)), 7);
		uint32_t SS2 = SS1 ^ ROTL32(A, 12);
		uint32_t TT1 = FF(A, B_, C, j) + D + SS2 + W1[j];
		uint32_t TT2 = GG(E, F, G, j) + H + SS1 + W[j];
		D = C;
		C = ROTL32(B_, 9);
		B_ = A;
		A = TT1;
		H = G;
		G = ROTL32(F, 19);
		F = E;
		E = P0(TT2);
	}

	V[0] ^= A; V[1] ^= B_; V[2] ^= C; V[3] ^= D;
	V[4] ^= E; V[5] ^= F;  V[6] ^= G; V[7] ^= H;
}


void iteration(const string& padded, uint32_t V[8]) {
	int n = padded.size() / 128;
	for (int i = 0; i < n; i++) {
		uint32_t B[16];
		for (int j = 0; j < 16; j++) {
			int pos = i * 128 + j * 8;
			if (pos + 8 > (int)padded.size()) {
				throw out_of_range("padded substring out of range");
			}
			string sub = padded.substr(pos, 8);
			//cout << "Debug substr: " << sub << endl;  
			try {
				B[j] = static_cast<uint32_t>(stoul(sub, nullptr, 16));
			}
			catch (const invalid_argument& e) {
				cerr << "Invalid hex substring: \"" << sub << "\"" << endl;
				throw;
			}
			catch (const out_of_range& e) {
				cerr << "Out of range hex substring: \"" << sub << "\"" << endl;
				throw;
			}

		}
		compress(V, B);
	}
}


string SM3Hash(string data, uint8_t prefix) {
	// 前缀（0x00 或 0x01） + 原始数据
	string new_data(1, prefix);
	new_data += data;
	// padding + SM3
	uint32_t IV[8] = {
		0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
		0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
	};
	string padded = padding(new_data);
	iteration(padded, IV);
	string hash = "";
	for (int i = 0; i < 8; i++) {
		char buf[9];
		sprintf_s(buf, "%08X", IV[i]);
		hash += buf;
	}
	return hash;
}

// Merkle 树结构类
class MerkleTree {
public:
	MerkleTree(const vector<string>& input_data) {
		build_tree(input_data);
	}

	string get_root() const {
		return levels.empty() ? "" : levels.back()[0];
	}
	vector<string> get_inclusion_proof(int index) const {
		if (index < 0 || index >= static_cast<int>(leaves.size())) {
			throw out_of_range("Inclusion proof index out of range");
		}

		vector<string> proof;
		int idx = index;
		for (size_t level = 0; level + 1 < levels.size(); ++level) {
			int sibling = (idx % 2 == 0) ? idx + 1 : idx - 1;
			// 加 sibling >= 0 的判断
			if (sibling >= 0 && sibling < static_cast<int>(levels[level].size())) {
				proof.push_back(levels[level][sibling]);
			}
			idx /= 2;
		}
		return proof;
	}

	// 非存在性证明：找相邻两个叶子的证明
	tuple<string, vector<string>, string, vector<string>>
		get_non_inclusion_proof(const string& target) const {
		if (leaves.empty()) {
			throw runtime_error("Tree is empty.");
		}

		auto it = lower_bound(leaves.begin(), leaves.end(), target);
		int idx_after = static_cast<int>(it - leaves.begin());

		if (it != leaves.end() && *it == target) {
			throw runtime_error("Target value exists in the tree.");
		}

		string before = (idx_after > 0) ? leaves[idx_after - 1] : "";
		string after = (idx_after < static_cast<int>(leaves.size())) ? leaves[idx_after] : "";

		vector<string> proof1 = before.empty() ? vector<string>{} : get_inclusion_proof(idx_after - 1);
		vector<string> proof2 = after.empty() ? vector<string>{} : get_inclusion_proof(idx_after);

		return make_tuple(before, proof1, after, proof2);
	}



private:
	vector<vector<string>> levels;
	vector<string> leaves;

	void build_tree(vector<string> input_data) {
		// 存储叶子内容排序副本
		leaves = input_data;
		sort(leaves.begin(), leaves.end());

		// 初始化叶子节点
		vector<string> current_level;
		for (const string& s : leaves) {
			current_level.push_back(SM3Hash(s, 0x00));
		}
		levels.push_back(current_level);

		while (current_level.size() > 1) {
			vector<string> next_level;
			for (size_t i = 0; i < current_level.size(); i += 2) {
				if (i + 1 == current_level.size()) {
					next_level.push_back(current_level[i]);
				}
				else {
					string combined = current_level[i] + current_level[i + 1];
					next_level.push_back(SM3Hash(combined, 0x01));
				}
			}
			levels.push_back(next_level);
			current_level = next_level;
		}
	}
};

int main() {
	vector<string> data;
	for (int i = 0; i < 100000; ++i) {
		data.push_back("data" + to_string(i));
	}
	try {
		auto start = high_resolution_clock::now();
		MerkleTree tree(data);
		auto end = high_resolution_clock::now();

		cout << "\n Merkle 树构建完成，耗时: "
			<< duration_cast<milliseconds>(end - start).count() << " ms\n";
		cout << "Merkle Root: " << tree.get_root() << "\n";


		int target = 12345;
		if (target >= 0 && target < data.size()) {
			try {
				auto proof = tree.get_inclusion_proof(target);
				cout << "\n存在性证明 (leaf index = " << target << "):\n";
				for (const auto& p : proof) cout << p << endl;
			}
			catch (const exception& e) {
				cerr << "Inclusion proof error: " << e.what() << endl;
			}
		}
		else {
			cerr << "目标索引越界: " << target << endl;
		}

		// ----------- 非存在性证明测试 -----------
		string not_exist = "data100001";
		try {
			auto [before, proof1, after, proof2] = tree.get_non_inclusion_proof(not_exist);
			cout << "\n非存在性证明 for \"" << not_exist << "\":\n";
			if (!before.empty()) {
				cout << "前邻: " << before << "\n路径:\n";
				for (const auto& p : proof1) cout << p << endl;
			}
			else {
				cout << "没有前邻（目标小于树中最小叶子）\n";
			}

			if (!after.empty()) {
				cout << "后邻: " << after << "\n路径:\n";
				for (const auto& p : proof2) cout << p << endl;
			}
			else {
				cout << "没有后邻（目标大于树中最大叶子）\n";
			}
		}
		catch (const exception& e) {
			cerr << "Non-inclusion proof error: " << e.what() << endl;
		}
		
	}
	catch (const std::exception& e) {
		cerr << "异常捕获: " << e.what() << endl;
	}


	return 0;
}
