import random
import hashlib
import time
import binascii

# SM2标准推荐曲线参数（sm2p256v1）
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

    # 模逆运算（扩展欧几里得算法）
    def _mod_inverse(self, a, p):
        if a == 0:
            raise ZeroDivisionError("Inverse of 0 is undefined")
        return pow(a, -1, p)

    # 椭圆曲线点加法
    def _point_add(self, P, Q):
        if P == (0, 0): return Q
        if Q == (0, 0): return P
        x1, y1 = P
        x2, y2 = Q

        if x1 == x2 and (y1 + y2) % self.p == 0:
            return (0, 0)

        if P == Q:
            l = (3 * x1 * x1 + self.a) * self._mod_inverse(2 * y1, self.p) % self.p
        else:
            l = (y2 - y1) * self._mod_inverse(x2 - x1, self.p) % self.p

        x3 = (l * l - x1 - x2) % self.p
        y3 = (l * (x1 - x3) - y1) % self.p
        return (x3, y3)

    # 点乘（倍点运算）
    def _point_mul(self, k, P):
        result = (0, 0)
        while k:
            if k & 1:
                result = self._point_add(result, P)
            P = self._point_add(P, P)
            k >>= 1
        return result

    # KDF（密钥派生函数）使用SM3（如不可用则回退SHA256）
    def _hash(self, data):
        try:
            sm3 = hashlib.new('sm3')
        except ValueError:
            sm3 = hashlib.sha256()  # fallback
        sm3.update(data)
        return sm3.digest()

    def _kdf(self, Z, klen):
        ct = 1
        output = b''
        while len(output) < klen:
            output += self._hash(Z + ct.to_bytes(4, 'big'))
            ct += 1
        return output[:klen]

    # 密钥对生成
    def generate_keypair(self):
        private_key = random.randint(1, self.n - 1)
        public_key = self._point_mul(private_key, self.G)
        return private_key, public_key

    # 公钥序列化
    def serialize_public_key(self, pub):
        return b'\x04' + pub[0].to_bytes(32, 'big') + pub[1].to_bytes(32, 'big')

    # SM2 加密
    def encrypt(self, public_key, msg):
        if isinstance(msg, str):
            msg = msg.encode()
        klen = len(msg)
        k = random.randint(1, self.n - 1)

        C1 = self._point_mul(k, self.G)
        C1_bytes = self.serialize_public_key(C1)

        S = self._point_mul(k, public_key)
        Z = self.serialize_public_key(S)
        t = self._kdf(Z, klen)
        if all(b == 0 for b in t):
            raise ValueError("KDF derived key is all-zero")

        C2 = bytes([m ^ t_i for m, t_i in zip(msg, t)])
        C3 = self._hash(msg + Z)
        return C1_bytes + C3 + C2

    # SM2 解密
    def decrypt(self, private_key, ciphertext):
        if ciphertext[0] != 0x04:
            raise ValueError("Unsupported C1 format")
        C1 = (int.from_bytes(ciphertext[1:33], 'big'), int.from_bytes(ciphertext[33:65], 'big'))
        C3 = ciphertext[65:97]
        C2 = ciphertext[97:]
        klen = len(C2)

        S = self._point_mul(private_key, C1)
        Z = self.serialize_public_key(S)
        t = self._kdf(Z, klen)
        if all(b == 0 for b in t):
            raise ValueError("KDF derived key is all-zero")

        msg = bytes([c ^ t_i for c, t_i in zip(C2, t)])
        u = self._hash(msg + Z)
        if u != C3:
            raise ValueError("Hash mismatch!")
        return msg



if __name__ == "__main__":
    sm2 = SM2()
    message = "SM2 encryption test."
    print("原始消息:", message)

    # 生成密钥对
    priv, pub = sm2.generate_keypair()
    print(f"私钥: {hex(priv)}")
    print(f"公钥: {binascii.hexlify(sm2.serialize_public_key(pub)).decode()}")

    # 加密
    t1 = time.time()
    cipher = sm2.encrypt(pub, message)
    t2 = time.time()
    print(f"加密耗时: {(t2 - t1) * 1000:.2f} ms")
    print(f"密文(hex): {binascii.hexlify(cipher).decode()}")

    # 解密
    t3 = time.time()
    plain = sm2.decrypt(priv, cipher)
    t4 = time.time()
    print(f"解密耗时: {(t4 - t3) * 1000:.2f} ms")
    print("解密结果:", plain.decode())
