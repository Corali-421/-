# SM3 实现与应用

本项目实现了国密 SM3 哈希算法的基础版本、优化版本，以及基于 SM3 的 Length Extension Attack 和 RFC6962 Merkle 树（含存在性证明与不存在性证明）。

目录：
- `SM3_basic.cpp` — SM3 基础实现
- `SM3_optimized.cpp` — SM3 优化实现
- `SM3_length_extension_attack.cpp` — Length Extension Attack
- `SM3_Merkle_Tree.cpp` — RFC6962 Merkle 树构建与证明

---

## 1. SM3 算法数学推导

SM3 哈希算法是一种中国国家密码管理局标准（GM/T 0004-2012），其内部结构与 SHA-256 类似，但采用不同的常量与消息扩展方式。

### 1.1 初始参数
SM3 使用 256 位初始向量（IV）：
$$
IV = (A_0,B_0,C_0,D_0,E_0,F_0,G_0,H_0)
$$
固定为：

7380166F 4914B2B9 172442D7 DA8A0600
A96F30BC 163138AA E38DEE4D B0FB0E4E

### 1.2 消息填充
消息长度 $l$（比特）填充至 $l \equiv 448 \pmod{512}$，再附加 64 位消息长度。

### 1.3 消息扩展
消息块 $B^{(j)}$ 被扩展为 $W_0, W_1, \dots, W_{67}$：
$$
W_j = M_j,\quad 0 \le j \le 15
$$
$$
W_j = P_1(W_{j-16} \oplus W_{j-9} \oplus (W_{j-3} \lll 15)) \oplus (W_{j-13} \lll 7) \oplus W_{j-6}
$$
其中：
$$
P_1(X) = X \oplus (X \lll 15) \oplus (X \lll 23)
$$

### 1.4 压缩函数
循环 64 轮：
$$
\begin{aligned}
TT_1 &= (A \lll 12) + E + (T_j \lll j) \\
SS_1 &= (TT_1 \lll 7) \oplus A \\
SS_2 &= SS_1 \oplus (A \lll 12) \\
TT_2 &= FF_j(A,B,C) + D + SS_2 + W'_j \\
E &= P_0(FF_j(E,F,G) + H + SS_1 + W_j)
\end{aligned}
$$
最终：
$$
V^{(j+1)} = V^{(j)} \oplus (A,B,C,D,E,F,G,H)
$$

---

### 1.5 基础测试结果
<img width="1315" height="133" alt="image" src="https://github.com/user-attachments/assets/33c7466e-4692-40c9-a30a-1e4b8165f602" />


## 2. SM3 优化实现

### 2.1 优化方向

1. **循环展开**：减少分支预测失败  
2. **预计算常量 $T_j$ 左移结果**  
3. **内存对齐优化**：减少缓存缺失  
4. **向量化优化**：利用 SIMD 指令加速

### 2.2 优化测试结果
<img width="1315" height="117" alt="image" src="https://github.com/user-attachments/assets/d0e74e8b-97dd-46f6-b9da-3d37b8feace7" />


## 3. Length Extension Attack

### 3.1 原理
SM3 是基于 Merkle–Damgård 结构的哈希函数，存在长度扩展攻击：
若已知 $H(m)$ 和 $|m|$，攻击者可以构造 $H(m \| pad(m) \| m')$，无需知道 $m$ 本身。

### 3.2 数学表示
已知：
$$
h = SM3(m)
$$
可以构造：
$$
h' = SM3_{IV=h}(m'')
$$
其中：
$$
m'' = pad(m) \| m'
$$

---

### 3.3 长度扩展攻击测试结果
<img width="1702" height="208" alt="image" src="https://github.com/user-attachments/assets/475f31b0-eff5-4fab-a57d-143d20523af2" />


## 4. RFC6962 Merkle 树

### 4.1 构建
叶子节点：
$$
L_i = SM3(0x00 \| data_i)
$$
内部节点：
$$
N = SM3(0x01 \| left \| right)
$$

树高：
$$
h = \lceil \log_2(n) \rceil
$$
本实现支持 10 万叶子节点。

---

### 4.3 存在性证明（Inclusion Proof）
给定叶子 $L_k$，证明其在树中：
- 提供从 $L_k$ 到根的哈希路径
- 验证方依次拼接并哈希，最终结果与 Merkle 根匹配

---

### 4.4 不存在性证明（Non-inclusion Proof）

#### 数学原理
RFC6962 的**不存在性证明**通过证明目标值在字典序中位于两个相邻已存在叶子之间，且不等于任一叶子值，从而证明该值不在树中。

设：
- $x$ 为要验证的目标值
- $L_i, L_{i+1}$ 为相邻两个叶子，且：
$$
L_i < x < L_{i+1}
$$
其中比较基于原始数据的字典序。

#### 证明内容
1. **相邻性证明**：给出 $L_i$ 和 $L_{i+1}$ 的存在性证明（从各自到 Merkle 根的路径）
2. **区间排除**：验证方确认：
   - $L_i$ 的值 < $x$
   - $L_{i+1}$ 的值 > $x$
   - $L_i$ 与 $L_{i+1}$ 在排序中紧邻

#### 验证算法伪代码
bool verifyNonInclusion(x, proof_Li, proof_Li1, root) {
    // 验证 L_i 在树中
    if (!verifyInclusion(proof_Li, root)) return false;

    // 验证 L_{i+1} 在树中
    if (!verifyInclusion(proof_Li1, root)) return false;

    // 验证区间关系
    if (!(proof_Li.leafValue < x && x < proof_Li1.leafValue))
        return false;

    return true;
}

数学表达

证明集合：

\[
\Pi = \{ (L_i, \text{path}_i), (L_{i+1}, \text{path}_{i+1}) \}
\]

验证条件：

\[
\begin{cases}
\text{verify}(\text{path}_i, L_i) = \text{root} \\
\text{verify}(\text{path}_{i+1}, L_{i+1}) = \text{root} \\
L_i < x < L_{i+1}
\end{cases}
\]

---

### 4.5 Merkle Tree构建与测试结果
<img width="1444" height="615" alt="image" src="https://github.com/user-attachments/assets/0eab843a-946e-4cee-ab1b-eae3c3ea734f" />
<img width="1437" height="577" alt="image" src="https://github.com/user-attachments/assets/fe29fa86-c15d-4dd4-b9c1-8276c58b8256" />
<img width="1369" height="538" alt="image" src="https://github.com/user-attachments/assets/542ae5e7-1906-4682-912d-dc0361ecd131" />


## 5. 编译与运行（C++）

### 5.1 编译

g++ SM3_basic.cpp -o sm3_basic
g++ SM3_optimized.cpp -o sm3_optimized
g++ SM3_length_extension_attack.cpp -o sm3_lenext
g++ SM3_Merkle_Tree.cpp -o sm3_merkle

### 5.2 运行
./sm3_basic
./sm3_optimized
./sm3_lenext
./sm3_merkle
