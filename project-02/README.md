# 🖼️ 基于 DCT + SVD 的图像水印系统

本项目实现了一套 **彩色图像水印嵌入与提取系统**，结合了频域处理中的 **离散余弦变换（DCT）** 与 **奇异值分解（SVD）**，并支持图像鲁棒性攻击测试，如翻转、平移、裁剪、调对比度与添加噪声。

---

## 📥 1. 水印嵌入（`embed_watermark.py`）

该脚本实现了基于 **DCT + SVD** 的图像水印嵌入流程，原理如下：

### 🚀 算法步骤：

1. 对原图每个通道进行 DCT，得到频域系数矩阵 `C_dct`
2. 选取左上角低频区域 `C_block`，执行 SVD：
C_block = U_C · S_C · V_C.T


3. 对水印图 `W` 进行 SVD：
W = U_W · S_W · V_W.T


4. 将水印奇异值嵌入原图奇异值：
S_E = S_C + α · S_W


- `α` 控制嵌入强度，越大越明显但可能影响图像质量

5. 重构频域区域并逆 DCT 生成带水印图像：
C_block' = U_C · S_E · V_C.T
C_watermarked = IDCT(C_dct')



### 📂 输入输出：

- 输入图片：
- `lena.png`：原始图像
- `logo.png`：灰度水印图（此处使用了山大的 logo）
- 输出结果：
- `output/watermarked_rgb.png`：嵌入水印后的彩色图像

---

## 📤 2. 水印提取（`extract_watermark.py`）

此脚本用于从原图与水印图中提取嵌入的水印内容。

### 提取流程：

1. 对原图 `C` 和带水印图 `C'` 分别进行 DCT，取相同区域：
C_block = U · S_C · V.T
C'_block = U · S'_C · V.T


2. 利用奇异值差异计算出嵌入的水印奇异值：
S_W = (S'_C − S_C) / α


3. 使用嵌入时的 U、V 矩阵恢复水印图像：
W^ = U · S_W · V.T



输出的 `W^` 即为提取出的水印图像。

---

## 🛡️ 3. 鲁棒性测试（`test_robustness.py`）

为了验证该系统对常见图像处理的鲁棒性，项目还内置了多种扰动攻击：

- 🔄 **翻转**：左右镜像
- 📐 **平移**：整体向右下移动 10 像素
- ✂️ **裁剪**：上下左右各裁剪 30 像素，仅保留中间区域
- 🎚️ **调对比度**：图像对比度增强为 1.5 倍
- 🌫️ **添加噪声**：添加高斯噪声（σ=10）

每种攻击之后，自动对扰动后的图像进行水印提取，并保存结果。

---

## ▶️ 快速运行

确保安装好依赖后，运行以下脚本完成嵌入与提取：

```bash
python embed_watermark.py            # 嵌入水印
python extract_watermark.py          # 提取水印
python run_attacks_and_extract.py    # 攻击测试 + 提取水印
📁 文件结构说明

├── embed_watermark.py             # 水印嵌入脚本
├── extract_watermark.py           # 水印提取脚本
├── test_robustness.py     # 鲁棒性测试脚本
├── lena.png                       # 示例原图
├── logo.png                       # 示例水印图
├── output/
│   ├── watermarked_rgb.png        # 嵌入后图像
│   ├── attacks/
│   │   ├── flip.png               # 各类攻击图像
│   │   └── ...
│   └── attacks_extracted/
│       ├── flip_extracted.png     # 对应提取出的水印
│       └── ...
