# SM2 加密算法 Python 实现与优化

本项目展示了 SM2 公钥密码算法的两种 Python 实现：

- ✅ **基础版**：使用最直观的椭圆曲线点加/点乘实现  
- 🚀 **高效版**：采用 Jacobian 坐标、Montgomery 梯子、窗口法与预计算表加速

---

## 📌 一、基础版算法说明

该实现使用经典的椭圆曲线加密方法：

- 🔁 点加法：`_point_add(P, Q)`
- 🔢 点乘法：`_point_mul(k, P)` 使用朴素的 **二进制展开法**
- 🔐 提供基本的 `encrypt()` 和 `decrypt()` 加解密接口

适合理解 SM2 基本原理，但性能较低，模逆频繁。

---

## ⚡ 二、高效版算法说明

在基础版的基础上，我们进行了如下优化：

### ✅ 优化技术

- 📐 **Jacobian 坐标**：避免模逆（Jacobian 点加/倍点）
- 📊 **固定点预计算表**：加速 `k*G` 计算
- 🪟 **窗口法**：滑动窗口减少倍点次数（默认窗口大小为 4）
- 🧗 **Montgomery 梯子法**：安全高效处理非固定点乘

### ✅ 支持场景

- 🧾 **加密解密**：`encrypt() / decrypt()`
- ✍️ **签名验签**：`sign() / verify()`

### 🚀 核心：Jacobian + 预计算加速效果图

<img width="1074" height="231" alt="Jacobian窗口法点乘" src="https://github.com/user-attachments/assets/805edfaa-30b5-477a-95ff-aff40bd8659d" />

---

## ⏱️ 三、运行测试效果

> 📌 代码可直接运行，含时间测试模块

### ⏳ 运行结果截图

基础版运行结果：

<img width="1527" height="238" alt="基础版运行时间" src="https://github.com/user-attachments/assets/b0542aac-6df9-4da3-82f4-7bd76d7e1022" />

优化版运行结果：

<img width="1468" height="291" alt="优化版运行时间" src="https://github.com/user-attachments/assets/792196c0-f01c-4ca9-b510-e219ad719a9d" />

### 📈 性能对比

| 操作       | 基础版耗时 | 高效版耗时 | 提升倍数 |
|------------|-------------|-------------|----------|
| 加密 Encrypt | ~26ms        | ~3ms         | 🟢 **约 9 倍** |
| 解密 Decrypt | ~13ms        | ~7ms         | 🟢 **约 2 倍** |

---

## 📦 四、运行方式

```bash
python sm2_basic.py      # 运行基础版
python sm2_optimized.py  # 运行高效优化版

