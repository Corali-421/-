#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <immintrin.h> 
using namespace std;
using namespace chrono;

//������ת��Ϊʮ�����ƺ���ʵ��
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

//ʮ������ת��Ϊ�����ƺ���ʵ��
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

//������ת��Ϊʮ���Ƶĺ���ʵ��
int BinToDec(string str) {
	int dec = 0;
	for (int i = 0; i < str.size(); i++) {
		dec += (str[i] - '0') * pow(2, str.size() - i - 1);
	}
	return dec;
}

//ʮ����ת��Ϊ�����Ƶĺ���ʵ��
string DecToBin(int str) {
	string bin = "";
	while (str >= 1) {
		bin = to_string(str % 2) + bin;
		str = str / 2;
	}
	return bin;
}

//ʮ������ת��Ϊʮ���Ƶĺ���ʵ��
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

//ʮ����ת��Ϊʮ�����Ƶĺ���ʵ��
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

string padding(string str) {//�����ݽ������ 
	string res = "";
	for (int i = 0; i < str.size(); i++) {
		res += DecToHex((int)str[i]);
	}
	int res_length = res.size() * 4;
	res += "8";
	while (res.size() % 128 != 112) {
		res += "0";//��0���������
	}
	string res_len = DecToHex(res_length);//���ڼ�¼���ݳ��ȵ��ַ���
	while (res_len.size() != 16) {
		res_len = "0" + res_len;
	}
	res += res_len;
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

string ModAdd(string str1, string str2) {//mod 2^32����ĺ���ʵ��
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

//void message_extension(const uint32_t B[16], uint32_t W[68], uint32_t W1[64]) {
//	for (int i = 0; i < 16; i++) W[i] = B[i];
//	for (int i = 16; i < 68; i++) {
//		uint32_t tmp = W[i - 16] ^ W[i - 9] ^ ROTL32(W[i - 3], 15);
//		W[i] = P1(tmp) ^ ROTL32(W[i - 13], 7) ^ W[i - 6];
//	}
//	for (int i = 0; i < 64; i++) {
//		W1[i] = W[i] ^ W[i + 4];
//	}
//}

void extension_SIMD(const uint32_t B[16], uint32_t W[68], uint32_t W1[64]) {
	// W[0..15] ֱ�Ӹ�ֵ
	for (int i = 0; i < 16; ++i)
		W[i] = B[i];

	// W[16..67]�������Ż�����������ͨ�߼�
	for (int j = 16; j < 68; ++j) {
		uint32_t tmp = W[j - 16] ^ W[j - 9] ^ ROTL32(W[j - 3], 15);
		W[j] = P1(tmp) ^ ROTL32(W[j - 13], 7) ^ W[j - 6];
	}

	// SIMD�Ż� W��[0..63] = W[i] �� W[i+4]
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
	int n = padded.size() / 8 / 16;
	for (int i = 0; i < n; i++) {
		uint32_t B[16];
		for (int j = 0; j < 16; j++) {
			B[j] = stoi(padded.substr(i * 128 + j * 8, 8), nullptr, 16);
		}
		compress(V, B);
	}
}

// ���ݸ�����Ϣ�Ͷ�ӦIV�����������Ϣ��������չ��������
void length_extension_attack(
	const string& original_msg,
	const uint32_t original_IV[8],
	const string& append_msg)
{
	// 1. ��ԭʼ��Ϣ���õ������ԭʼ��Ϣ���ȣ���λbit��
	string padded_original = padding(original_msg);
	int original_bit_len = padded_original.size() * 4; // hex char count * 4 = bits

	// 2. α�����: ���ǹ���α����Ϣ = ԭʼ��Ϣ + ԭʼ��Ϣ��� + ������Ϣ��append_msg��
	// ʵ���ϣ�����ģ����ϢΪ�� ԭʼ��Ϣ + padding + append_msg
	// ��padding�����ԭʼ��Ϣ���ȵ�
	// �������ǲ���Ҫԭʼ��Ϣ��ֻ��Ҫ��������Ϣ��䣬���� = original_bit_len + append_msg_len*8

	// 3. ƴ��������Ϣ�����ֶ�padding����append_msg������ original_bit_len + append_msg.size()*8 ���
	// ������padding����������ַ����ģ���Ҫ��дһ��padding�����ܽ��ճ��Ȳ���

	// ���� padding ������������Ϣ�� total_len(λ)
	auto padding_with_len = [](const string& msg, int total_bit_len) -> string {
		string res = "";
		for (char c : msg) {
			res += DecToHex((int)c);
		}
		res += "8";
		while (res.size() % 128 != 112) {
			res += "0";
		}
		string len_hex = DecToHex(total_bit_len);
		while (len_hex.size() < 16) {
			len_hex = "0" + len_hex;
		}
		res += len_hex;
		return res;
		};

	string appended_padded = padding_with_len(append_msg, original_bit_len + append_msg.size() * 8);

	// 4. ����ԭʼIV���ڲ���
	uint32_t V[8];
	memcpy(V, original_IV, sizeof(uint32_t) * 8);

	// 5. ����������Ϣ�����ѹ��
	iteration(appended_padded, V);

	// 6. ��ӡα����Ϣ�Ĺ�ϣ
	cout << "α����Ϣ�Ĺ�ϣֵ: ";
	for (int i = 0; i < 8; i++) {
		printf("%08X", V[i]);
		if (i != 7) cout << " ";
	}
	cout << endl;

	// 7. Ϊ������֤������Դ�ӡα����������Ϣ(ԭʼ��Ϣ + padding + ����Ϣ)��hex�ַ���
	cout << "α��������Ϣ (hex): " << endl;
	string forged_msg_hex = "";
	for (char c : original_msg) forged_msg_hex += DecToHex((int)c);
	// ���ԭʼ��Ϣ��padding��hex�ַ�����
	forged_msg_hex += padded_original.substr(original_msg.size() * 2); // ��ȥԭʼ��Ϣ�����hex��ֻ��padding����
	// ���α���������Ϣ����padding
	forged_msg_hex += appended_padded;
	cout << forged_msg_hex << endl;
}

int main() {
	string original = "abc";
	string append = "def";

	cout << "ԭʼ��Ϣ: " << original << endl;

	// ��ʼ�� IV
	uint32_t IV[8] = {
		0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
		0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
	};

	// ����ԭʼ��Ϣ�� hash
	string padded = padding(original);
	iteration(padded, IV);

	cout << "ԭʼ��Ϣ��ϣ: ";
	for (int i = 0; i < 8; i++) {
		printf("%08X ", IV[i]);
	}
	cout << endl;

	// ���г�����չ����
	length_extension_attack(original, IV, append);

	return 0;
}
