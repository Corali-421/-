SM2 加密算法 Python 实现和优化

本项目展示了 SM2 加密算法的 Python 实现，包括一个 基础版（简单递归点加/点乘）和一个 高效版（使用了 Jacobian 坐标、约算乘法、窗口预计算表）。
一、基础版算法

使用了数字密码中最基本的“点加法、点乘法”

点加采用抽象函数 _point_add()

点乘采用了简单的二进制开展法 _point_mul()

提供 encrypt() / decrypt() 基本功能

二、高效版算法

优化技术

使用 Jacobian 坐标系 避免频繁的模逆

实现 固定点预计算表 加速 k*G

采用 窗口法 （默认窗口 4 位）

非固定点使用 Montgomery 梯子法

实现场景

加密和解密: encrypt() / decrypt()

签名和验签: sign() / verify()

Jacobian + 窗口预计算
<img width="1074" height="231" alt="image" src="https://github.com/user-attachments/assets/805edfaa-30b5-477a-95ff-aff40bd8659d" />

代码可以直接运行，测试时间如下：
<img width="1527" height="238" alt="image" src="https://github.com/user-attachments/assets/b0542aac-6df9-4da3-82f4-7bd76d7e1022" />
<img width="1468" height="291" alt="image" src="https://github.com/user-attachments/assets/792196c0-f01c-4ca9-b510-e219ad719a9d" />  
加密时间提高约9倍，解密时间约2倍
