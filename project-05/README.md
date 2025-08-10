# SM2 加密算法 Python 实现与优化

本项目展示了 SM2 公钥密码算法的两种 Python 实现：

- **基础版**：使用最直观的椭圆曲线点加/点乘实现  
- **高效版**：采用 Jacobian 坐标、Montgomery 梯子、窗口法与预计算表加速

---

## SM2签名算法简介

SM2签名算法基于椭圆曲线离散对数问题（ECDLP），核心步骤为：

1. 随机选择整数`k ∈ [1, n-1]`

2. 计算椭圆曲线点 `(x_1, y_1) = kG`，其中`G`为基点，`n`为基点阶  

3. 计算 `r = (e + x_1) mod n`，`e = H(M)`是消息摘要  

4. 计算 `s = ((1 + d)^{-1} * (k - r*d)) mod n`，`d`为私钥  

5. 签名为 `(r, s)`  

验签过程则用公钥`P`和签名验证`r`是否满足：

\[
R = (e + x) \bmod n = r
\]

其中 `(x, y) = sG + tP`，`t = (r + s) mod n`

---

## 签名算法数学推导

给定消息`M`，计算消息摘要 `e = H(M)`。签名过程：

\[
\begin{cases}
k \xleftarrow{\$} [1, n-1] \\
(x_1, y_1) = kG \\
r = (e + x_1) \bmod n \\
s = (1 + d)^{-1} (k - r d) \bmod n
\end{cases}
\]

其中`d`是私钥。

验签时计算：

\[
t = (r + s) \bmod n \\
(x, y) = sG + tP \\
R = (e + x) \bmod n
\]

若 `R = r`，签名有效。

---

##  一、基础版算法说明

该实现使用经典的椭圆曲线加密方法：

- 点加法：`_point_add(P, Q)`
- 点乘法：`_point_mul(k, P)` 使用朴素的 **二进制展开法**
- 提供基本的 `encrypt()` 和 `decrypt()` 加解密接口

适合理解 SM2 基本原理，但性能较低，模逆频繁。

---

## 二、高效版算法说明

在基础版的基础上，我们进行了如下优化：

### 优化技术

- **Jacobian 坐标**：避免模逆（Jacobian 点加/倍点）
- **固定点预计算表**：加速 `k*G` 计算
- **窗口法**：滑动窗口减少倍点次数（默认窗口大小为 4）
- **Montgomery 梯子法**：安全高效处理非固定点乘

### 支持场景

- **加密解密**：`encrypt() / decrypt()`
- **签名验签**：`sign() / verify()`

### 核心：Jacobian + 预计算加速效果图

<img width="1074" height="231" alt="Jacobian窗口法点乘" src="https://github.com/user-attachments/assets/805edfaa-30b5-477a-95ff-aff40bd8659d" />

---

## 三、运行测试效果

> 代码可直接运行，含时间测试模块

### 运行结果截图

基础版运行结果：

<img width="1527" height="238" alt="基础版运行时间" src="https://github.com/user-attachments/assets/b0542aac-6df9-4da3-82f4-7bd76d7e1022" />

优化版运行结果：

<img width="1468" height="291" alt="优化版运行时间" src="https://github.com/user-attachments/assets/792196c0-f01c-4ca9-b510-e219ad719a9d" />

### 性能对比

| 操作       | 基础版耗时 | 高效版耗时 | 提升倍数 |
|------------|-------------|-------------|----------|
| 加密 Encrypt | ~26ms        | ~3ms         | 🟢 **约 9 倍** |
| 解密 Decrypt | ~13ms        | ~7ms         | 🟢 **约 2 倍** |

---

## 📦 四、运行方式

```bash
python sm2_basic.py      # 运行基础版
python sm2_optimized.py  # 运行高效优化版
```

# 签名算法的误用做poc验证-推导

## 签名误用场景数学分析

### 场景1：泄露随机数k导致私钥泄露

已知`k`，由签名关系：

\[
s = (1 + d)^{-1} (k - r d) \implies k = s (1 + d) + r d \mod n
\]

从中可解出私钥`d`。

---

### 场景2：重用k导致私钥泄露

对两个消息签名，使用同一`k`：

\[
\begin{cases}
s_1 = (1 + d)^{-1} (k - r_1 d) \\
s_2 = (1 + d)^{-1} (k - r_2 d)
\end{cases}
\]

两式相减：

\[
s_1 - s_2 = (1 + d)^{-1} d (r_2 - r_1) \Rightarrow d = \frac{s_1 - s_2}{r_2 - r_1 - (s_1 - s_2)} \mod n
\]

---

### 场景3：不同用户重用同一k导致私钥泄露

不同私钥`d_A, d_B`，重用同一`k`，可利用签名值推导对方私钥，数学关系较复杂，基于相似的线性方程求解。

---

### 场景4：ECDSA与Schnorr共用(d,k)导致私钥泄露

若ECDSA和Schnorr签名中同用`(d,k)`，可利用两个签名的线性关系推导私钥：

\[
\begin{cases}
s_E = k^{-1}(e + d r) \\
s_S = k + e_s d
\end{cases}
\]

解得：

\[
d = \frac{s_E s_S - e}{r + s_E e_s} \bmod n
\]

---

## 代码结果展示
<img width="1596" height="570" alt="image" src="https://github.com/user-attachments/assets/883a8c82-a500-48ed-9b1f-e39e06bd835d" />

# 伪造中本聪数字签名

---

## 一、实验背景与目标

ECDSA（椭圆曲线数字签名算法）是区块链和加密货币中最广泛使用的数字签名算法。  
本实验实现了ECDSA算法及其签名和验签流程，重点验证并演示“中本聪无消息签名伪造攻击”（Satoshi forgery），该攻击能在不知私钥且不知原始消息的情况下伪造签名摘要对，通过验证。

---

## 二、ECDSA算法简介

ECDSA基于椭圆曲线上的离散对数问题，其核心签名过程为：

1. 随机选取 `k ∈ [1, n-1]`  
2. 计算点 `R = kG = (x_1, y_1)`  
3. 计算 `r = x_1 mod n`，若`r=0`重新选`k`  
4. 计算消息摘要 `e = H(M)`  
5. 计算 `s = k^{-1} (e + d r) mod n`，其中`d`是私钥  
6. 签名为 `(r, s)`  

验签过程：

1. 计算 `w = s^{-1} mod n`  
2. 计算 `u1 = e w mod n`，`u2 = r w mod n`  
3. 计算点 `P = u1 G + u2 Q`，`Q = dG`是公钥  
4. 签名有效当且仅当 `r ≡ x(P) mod n`

---

## 三、无消息签名伪造攻击原理数学推导

该攻击思路源于Satoshi Nakamoto提出，攻击者不需私钥和消息，只需随机选`u`和`v`，构造伪造签名和对应摘要：

1. 选随机数 `u, v ∈ [1, n-1]`  
2. 计算椭圆曲线点  
\[
R = uG + vQ = (x_R, y_R)
\]  
3. 计算签名参数  
\[
r = x_R \bmod n
\]  
\[
s = r \cdot v^{-1} \bmod n
\]  
4. 计算摘要  
\[
e = u \cdot r \cdot v^{-1} \bmod n
\]  

验证时：

\[
w = s^{-1} = v r^{-1} \bmod n
\]

\[
u_1 = e w = u r v^{-1} \cdot v r^{-1} = u \bmod n
\]

\[
u_2 = r w = r \cdot v r^{-1} = v \bmod n
\]

计算：

\[
u_1 G + u_2 Q = u G + v Q = R
\]

所以验签成功，伪造签名 `(r, s)` 和摘要 `e`通过验证。

---

## 四、代码实现思路

- 实现了基于secp256k1曲线的椭圆曲线加法、倍点及标量乘法（double-and-add算法）  
- 实现了ECDSA密钥对生成、签名和验签  
- `pretend`方法实现中本聪无消息伪造攻击：随机生成`u, v`，计算伪造签名和对应摘要  
- 额外实现无消息的验签方法，验证伪造签名的合法性  

---

## 五、实验结果与演示

- 成功生成ECDSA密钥对（私钥、公钥）  
- 正常消息签名及验签，验证签名有效  
- 使用`pretend`方法生成伪造签名 `(r, s)` 和摘要 `e`  
- 验证伪造签名时验签成功，说明攻击成立  

示例输出：
<img width="1675" height="612" alt="image" src="https://github.com/user-attachments/assets/4fd94ec2-a2bb-43be-b98a-a07e247ecc19" />


