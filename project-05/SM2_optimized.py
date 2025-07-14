import random
import hashlib
import time

# SM2推荐曲线参数
P = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF
A = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC
B = 0x28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93
N = 0xFFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123
Gx = 0x32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7
Gy = 0xBC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0

class SM2:
    def __init__(self):
        self.p = P
        self.a = A
        self.b = B
        self.n = N
        self.G = (Gx, Gy)
        self.window_size = 4
        self.precompute_table = self._precompute_fixed_point(self.G, self.window_size)

    # --------- 椭圆曲线基础与点乘（雅可比+预计算+窗口法） ---------
    def _mod_inverse(self, a, p):
        return pow(a, -1, p)

    def _jacobian_add(self, P, Q):
        if P[2] == 0:
            return Q
        if Q[2] == 0:
            return P

        X1, Y1, Z1 = P
        X2, Y2, Z2 = Q

        Z1Z1 = (Z1 * Z1) % self.p
        Z2Z2 = (Z2 * Z2) % self.p

        U1 = (X1 * Z2Z2) % self.p
        U2 = (X2 * Z1Z1) % self.p

        S1 = (Y1 * Z2 * Z2Z2) % self.p
        S2 = (Y2 * Z1 * Z1Z1) % self.p

        H = (U2 - U1) % self.p
        R = (S2 - S1) % self.p

        if H == 0:
            if R == 0:
                return self._jacobian_double(P)
            else:
                return (0, 1, 0)

        HH = (H * H) % self.p
        HHH = (H * HH) % self.p
        V = (U1 * HH) % self.p

        X3 = (R * R - HHH - 2 * V) % self.p
        Y3 = (R * (V - X3) - S1 * HHH) % self.p
        Z3 = (H * Z1 * Z2) % self.p

        return (X3, Y3, Z3)

    def _jacobian_double(self, P):
        X1, Y1, Z1 = P
        if Z1 == 0 or Y1 == 0:
            return (0, 1, 0)

        YY = (Y1 * Y1) % self.p
        S = (4 * X1 * YY) % self.p
        M = (3 * X1 * X1 + self.a * ((Z1 * Z1) % self.p) ** 2) % self.p
        X3 = (M * M - 2 * S) % self.p
        Y3 = (M * (S - X3) - 8 * (YY * YY % self.p)) % self.p
        Z3 = (2 * Y1 * Z1) % self.p

        return (X3, Y3, Z3)

    def _jacobian_to_affine(self, P):
        X, Y, Z = P
        if Z == 0:
            return (0, 0)
        Z_inv = self._mod_inverse(Z, self.p)
        Z_inv_sq = (Z_inv * Z_inv) % self.p
        x = (X * Z_inv_sq) % self.p
        y = (Y * Z_inv_sq * Z_inv) % self.p
        return (x, y)

    def _jacobian_mul_scalar(self, k, P):
        R = (0, 1, 0)
        Q = P
        for i in reversed(range(k.bit_length())):
            R = self._jacobian_double(R)
            if (k >> i) & 1:
                R = self._jacobian_add(R, Q)
        return R

    def _precompute_fixed_point(self, base, window_size):
        base_jac = (base[0], base[1], 1)
        table = []
        max_idx = 2 ** window_size
        for i in range(1, max_idx, 2):
            pt = self._jacobian_mul_scalar(i, base_jac)
            table.append(pt)
        return table

    def _point_mul_fixed(self, k, table, window_size):
        R = (0, 1, 0)
        k_bin = bin(k)[2:]
        i = 0
        while i < len(k_bin):
            if k_bin[i] == '0':
                R = self._jacobian_double(R)
                i += 1
            else:
                j = min(window_size, len(k_bin) - i)
                while j > 1 and int(k_bin[i:i+j], 2) % 2 == 0:
                    j -= 1
                val = int(k_bin[i:i+j], 2)
                idx = (val - 1) // 2
                for _ in range(j):
                    R = self._jacobian_double(R)
                R = self._jacobian_add(R, table[idx])
                i += j
        return R

    def _montgomery_ladder(self, k, P):
        R0 = (0, 1, 0)
        R1 = (P[0], P[1], 1)
        for i in reversed(range(k.bit_length())):
            bit = (k >> i) & 1
            if bit == 0:
                R1 = self._jacobian_add(R0, R1)
                R0 = self._jacobian_double(R0)
            else:
                R0 = self._jacobian_add(R0, R1)
                R1 = self._jacobian_double(R1)
        return R0

    def _point_mul(self, k, P):
        if P == self.G:
            R_jac = self._point_mul_fixed(k, self.precompute_table, self.window_size)
            return self._jacobian_to_affine(R_jac)
        else:
            R_jac = self._montgomery_ladder(k, P)
            return self._jacobian_to_affine(R_jac)

    # --------- 哈希和KDF ---------
    def _hash(self, data):
        try:
            h = hashlib.new('sm3')
        except:
            h = hashlib.sha256()
        h.update(data)
        return h.digest()

    def _kdf(self, Z, klen):
        ct = 1
        output = b''
        while len(output) < klen:
            output += self._hash(Z + ct.to_bytes(4, 'big'))
            ct += 1
        return output[:klen]

    # --------- SM2 密钥生成 ---------
    def generate_keypair(self):
        d = random.randint(1, self.n - 1)
        P = self._point_mul(d, self.G)
        return d, P

    def serialize_public_key(self, P):
        return b'\x04' + P[0].to_bytes(32, 'big') + P[1].to_bytes(32, 'big')

    # --------- SM2 加密 ---------
    def encrypt(self, public_key, msg):
        if isinstance(msg, str): 
            msg = msg.encode()
        klen = len(msg)
        k = random.randint(1, self.n - 1)

        C1 = self._point_mul(k, self.G)
        C1_bytes = self.serialize_public_key(C1)

        S = self._point_mul(k, public_key)
        x2_bytes = S[0].to_bytes(32, 'big')
        y2_bytes = S[1].to_bytes(32, 'big')

        t = self._kdf(x2_bytes + y2_bytes, klen)
        if all(b == 0 for b in t):
            raise ValueError("KDF = 0")

        C2 = bytes([m ^ t_i for m, t_i in zip(msg, t)])
        C3 = self._hash(x2_bytes + msg + y2_bytes)

        return C1_bytes + C3 + C2

    # --------- SM2 解密 ---------
    def decrypt(self, private_key, ciphertext):
        if ciphertext[0] != 0x04:
            raise ValueError("Invalid ciphertext format")
        C1 = (int.from_bytes(ciphertext[1:33], 'big'), int.from_bytes(ciphertext[33:65], 'big'))
        C3 = ciphertext[65:97]
        C2 = ciphertext[97:]

        S = self._point_mul(private_key, C1)
        x2_bytes = S[0].to_bytes(32, 'big')
        y2_bytes = S[1].to_bytes(32, 'big')

        t = self._kdf(x2_bytes + y2_bytes, len(C2))
        if all(b == 0 for b in t):
            raise ValueError("KDF = 0")

        M = bytes([c ^ t_i for c, t_i in zip(C2, t)])
        u = self._hash(x2_bytes + M + y2_bytes)

        if u != C3:
            raise ValueError("Hash verification failed")

        return M

    # --------- SM2 签名 ---------
    def sign(self, private_key, msg):
        if isinstance(msg, str):
            msg = msg.encode()
        e = int.from_bytes(self._hash(msg), 'big')
        while True:
            k = random.randint(1, self.n - 1)
            x1, y1 = self._point_mul(k, self.G)
            r = (e + x1) % self.n
            if r == 0 or r + k == self.n:
                continue
            s = (self._mod_inverse(1 + private_key, self.n) * (k - r * private_key)) % self.n
            if s == 0:
                continue
            return (r, s)

    # --------- SM2 验签 ---------
    def verify(self, public_key, msg, signature):
        if isinstance(msg, str):
            msg = msg.encode()
        r, s = signature
        if not (1 <= r < self.n and 1 <= s < self.n):
            return False
        e = int.from_bytes(self._hash(msg), 'big')
        t = (r + s) % self.n
        if t == 0:
            return False
        x1, y1 = self._point_mul(s, self.G)
        x2, y2 = self._point_mul(t, public_key)
        x, y = self._jacobian_to_affine(self._jacobian_add((x1, y1, 1), (x2, y2, 1)))
        R = (e + x) % self.n
        return R == r

if __name__ == "__main__":
    sm2 = SM2()
    msg = "Hello SM2 with precomputed jacobian window!"

    priv, pub = sm2.generate_keypair()
    print(f"私钥: {hex(priv)}")
    print(f"公钥: ({hex(pub[0])}, {hex(pub[1])})")

    # 加密时间测试
    t1 = time.time()
    ciphertext = sm2.encrypt(pub, msg)
    t2 = time.time()
    print(f"加密耗时: {(t2 - t1)*1000:.2f} ms")
    print(f"密文长度: {len(ciphertext)}")

    # 解密时间测试
    t3 = time.time()
    plaintext = sm2.decrypt(priv, ciphertext)
    t4 = time.time()
    print(f"解密耗时: {(t4 - t3)*1000:.2f} ms")
    print(f"解密结果: {plaintext.decode()}")

    # 签名时间测试
    t5 = time.time()
    signature = sm2.sign(priv, msg)
    t6 = time.time()
    print(f"签名耗时: {(t6 - t5)*1000:.2f} ms")
    print(f"签名: r={hex(signature[0])}, s={hex(signature[1])}")

    # 验签时间测试
    t7 = time.time()
    valid = sm2.verify(pub, msg, signature)
    t8 = time.time()
    print(f"验签耗时: {(t8 - t7)*1000:.2f} ms")
    print(f"验签结果: {valid}")