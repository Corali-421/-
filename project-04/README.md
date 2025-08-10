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
目的：减少循环中分支判断的次数，降低 CPU 分支预测失败的风险，提升流水线执行效率。

原理：循环展开通过将循环体内的多次迭代合并为一组连续操作，减少循环控制（如跳转、计数器更新）的开销。

效果：CPU 在执行时不必频繁判断循环终止条件，避免分支预测失败带来的流水线刷新，从而提高执行速度。
2. **预计算常量 $T_j$ 左移结果**
代码中，compress 函数里计算了这段：

uint32_t SS1 = ROTL32((ROTL32(A, 12) + E + ROTL32(T(j), j)), 7);
每轮调用 T(j) 返回两个固定常量之一（0x79CC4519 或 0x7A879D8A），且需要循环左移 j 位。

目前实现每次迭代动态调用 ROTL32(T(j), j)，这里可以通过提前预计算所有 T_j 左移的结果，存入静态数组。

预计算后，compress 函数内直接访问预先计算好的值，避免在热循环里执行重复的位移操作。

这减少了 CPU 运算指令，降低时钟周期消耗，提高函数性能。
3. **内存对齐优化**：减少缓存缺失

函数extension_SIMD 中用到了 AVX2 的 256 位加载和存储指令：

__m256i v1 = _mm256_loadu_si256((__m256i*)(W + i));
__m256i v2 = _mm256_loadu_si256((__m256i*)(W + i + 4));
_mm256_loadu_si256 是非对齐加载，虽然支持非对齐地址，但对齐加载（_mm256_load_si256）性能更优。

如果将数组 W 和 W1 保证在内存中以 32 字节边界对齐（如用 alignas(32) uint32_t W[68];），可以改用对齐加载指令。

对齐访问可减少缓存缺失和额外访存延迟，提升 SIMD 指令执行效率。

此外，内存对齐还有利于 CPU 缓存行的高效利用，减少数据访问瓶颈。  
4. **向量化优化**：利用 SIMD 指令加速

SM3 算法中，消息扩展包含两个数组：W[68] 和 W1[64]。

其中，W[0..15] 直接由输入消息块赋值，W[16..67] 通过复杂的非线性变换计算得到。

W1[0..63] 通过按位异或 W[i] ^ W[i+4] 得到。

传统实现

W1 的计算通常使用循环逐元素按位异或，效率受限。

优化实现

利用 AVX2 指令集的 256 位 SIMD 寄存器一次处理 8 个 uint32_t 元素，实现向量化的异或操作。

通过 _mm256_loadu_si256 加载 W[i] 和 W[i+4]，用 _mm256_xor_si256 执行并行异或，最后用 _mm256_storeu_si256 存回 W1。

这显著减少了循环迭代次数，提高数据处理吞吐量。

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
