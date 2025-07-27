import random
import hashlib

# 定义椭圆曲线参数 (secp256k1 - 比特币使用的曲线)
P = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
A = 0
B = 7
N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
Gx = 0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
Gy = 0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8

class EllipticCurve:
    def __init__(self, p, a, b, n, gx, gy):
        self.p = p
        self.a = a
        self.b = b
        self.n = n
        self.G = (gx, gy)
    
    def is_on_curve(self, point):
        """检查点是否在椭圆曲线上"""
        if point is None:
            return True
        x, y = point
        return (y * y - x * x * x - self.a * x - self.b) % self.p == 0
    
    def add(self, P, Q):
        """椭圆曲线点加法"""
        if P is None:
            return Q
        if Q is None:
            return P
        x1, y1 = P
        x2, y2 = Q
        
        if x1 == x2 and y1 != y2:
            return None
        
        if x1 == x2:
            m = (3 * x1 * x1 + self.a) * pow(2 * y1, -1, self.p) % self.p
        else:
            m = (y2 - y1) * pow(x2 - x1, -1, self.p) % self.p
        
        x3 = (m * m - x1 - x2) % self.p
        y3 = (m * (x1 - x3) - y1) % self.p
        return (x3, y3)
    
    def mul(self, k, P):
        """椭圆曲线标量乘法 (double-and-add算法)"""
        result = None
        current = P
        
        while k:
            if k & 1:
                result = self.add(result, current)
            current = self.add(current, current)
            k >>= 1
        return result
    
    def mul_inv(self, a, modulus):
        """模逆计算 (扩展欧几里得算法)"""
        if a == 0:
            return 0
        lm, hm = 1, 0
        low, high = a % modulus, modulus
        while low > 1:
            r = high // low
            nm, new = hm - lm * r, high - low * r
            hm, lm, high, low = lm, nm, low, new
        return lm % modulus


class ECDSA:
    def __init__(self, curve):
        self.curve = curve
    
    def generate_keypair(self):
        """生成密钥对"""
        d = random.randint(1, self.curve.n - 1)
        Q = self.curve.mul(d, self.curve.G)
        return d, Q
    
    def sign(self, d, message):
        """ECDSA签名"""
        k = random.randint(1, self.curve.n - 1)
        R = self.curve.mul(k, self.curve.G)
        r = R[0] % self.curve.n
        if r == 0:
            return self.sign(d, message)  # 重新选择k
        
        e = self.hash_message(message)
        s = (self.curve.mul_inv(k, self.curve.n) * (e + d * r)) % self.curve.n
        if s == 0:
            return self.sign(d, message)  # 重新选择k
        
        return (r, s)
    
    def verify(self, Q, message, signature):
        """ECDSA验证"""
        r, s = signature
        
        # 检查签名分量范围
        if not (1 <= r < self.curve.n and 1 <= s < self.curve.n):
            return False
        
        e = self.hash_message(message)
        w = self.curve.mul_inv(s, self.curve.n)
        u1 = (e * w) % self.curve.n
        u2 = (r * w) % self.curve.n
        
        # 计算点 u1*G + u2*Q
        P1 = self.curve.mul(u1, self.curve.G)
        P2 = self.curve.mul(u2, Q)
        R_prime = self.curve.add(P1, P2)
        
        if R_prime is None:
            return False
        
        return r == R_prime[0] % self.curve.n
    
    def hash_message(self, message):
        """消息哈希函数 (SHA-256)"""
        if isinstance(message, str):
            message = message.encode('utf-8')
        return int(hashlib.sha256(message).hexdigest(), 16) % self.curve.n
    
    def ver_no_m(self, Q, e, signature):
        """无消息验证 (直接使用摘要e)"""
        r, s = signature
        
        # 检查签名分量范围
        if not (1 <= r < self.curve.n and 1 <= s < self.curve.n):
            return False
        
        w = self.curve.mul_inv(s, self.curve.n)
        u1 = (e * w) % self.curve.n
        u2 = (r * w) % self.curve.n
        
        # 计算点 u1*G + u2*Q
        P1 = self.curve.mul(u1, self.curve.G)
        P2 = self.curve.mul(u2, Q)
        R_prime = self.curve.add(P1, P2)
        
        if R_prime is None:
            return False
        
        return r == R_prime[0] % self.curve.n
    
    def pretend(self, Q):
        """Satoshi无消息签名伪造"""
        # 1. 选择任意的u和v
        u = random.randint(1, self.curve.n - 1)
        v = random.randint(1, self.curve.n - 1)
        
        # 2. 计算点 R = u*G + v*Q
        P1 = self.curve.mul(u, self.curve.G)
        P2 = self.curve.mul(v, Q)
        R = self.curve.add(P1, P2)
        
        if R is None:
            return self.pretend(Q)  # 重新选择u,v
        
        r = R[0] % self.curve.n
        if r == 0:
            return self.pretend(Q)  # 重新选择u,v
        
        # 3. 计算 s = r * v^(-1) mod n
        v_inv = self.curve.mul_inv(v, self.curve.n)
        s = (r * v_inv) % self.curve.n
        if s == 0:
            return self.pretend(Q)  # 重新选择u,v
        
        # 4. 计算 e = u * r * v^(-1) mod n
        e = (u * r * v_inv) % self.curve.n
        
        return e, (r, s)


def main():
    # 初始化椭圆曲线和ECDSA
    curve = EllipticCurve(P, A, B, N, Gx, Gy)
    ecdsa = ECDSA(curve)
    
    print("=== ECDSA数字签名演示 ===")
    
    # 生成密钥对
    private_key, public_key = ecdsa.generate_keypair()
    print(f"私钥: {hex(private_key)}")
    print(f"公钥: ({hex(public_key[0])}, {hex(public_key[1])})")
    
    # 签名和验证
    message = "区块链安全技术"
    signature = ecdsa.sign(private_key, message)
    valid = ecdsa.verify(public_key, message, signature)
    print(f"\n消息: '{message}'")
    print(f"签名: (r={hex(signature[0])}, s={hex(signature[1])})")
    print(f"验证结果: {'有效' if valid else '无效'}")
    
    # Satoshi无消息签名伪造
    print("\n=== Satoshi无消息签名伪造攻击 ===")
    forged_e, forged_signature = ecdsa.pretend(public_key)
    
    print("伪造的签名:")
    print(f"r = {hex(forged_signature[0])}")
    print(f"s = {hex(forged_signature[1])}")
    print(f"e = {hex(forged_e)}")
    
    # 验证伪造的签名
    valid_forgery = ecdsa.ver_no_m(public_key, forged_e, forged_signature)
    print(f"\n伪造签名验证结果: {'成功' if valid_forgery else '失败'}")
    
    # 解释攻击原理
    print("\n=== 攻击原理分析 ===")
    print("1. 攻击者选择任意的u和v值")
    print("2. 计算点 R = u*G + v*Q")
    print("3. 设置 r = x(R) mod n")
    print("4. 计算 s = r * v⁻¹ mod n")
    print("5. 计算 e = u * r * v⁻¹ mod n")
    print("6. 伪造的签名(r, s)和摘要e能通过验证")
    print("\n关键点: 攻击者不需要知道私钥或原始消息，就能创建有效的签名摘要对")


if __name__ == "__main__":
    main()