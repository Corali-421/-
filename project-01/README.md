SM4_basic.cpp 实现了SM4算法的基础版本，完整遵循SM4标准流程，包括密钥扩展、16轮迭代加密和解密过程。该版本采用纯软件实现，结构清晰，便于理解算法细节，作为后续优化的基准。  
为便于观察和时间的稳定，本实验计算一个block，10000次加（解）密的时间
![image](https://github.com/user-attachments/assets/a376957a-161b-4e04-959e-e56c3f6bb11c)
SM4_Ttable.cpp 将S盒查找与线性变换L合并，预先计算生成T-Table（查找表），减少运算过程中的重复计算。通过表驱动方式，实现一次内存访问替代多次运算，时间如下：
![image](https://github.com/user-attachments/assets/ef294018-f5c4-4c64-811c-87933ccee40b)
SM4_multithreading_SIMD.cpp 在使用查找表的基础上，进一步采用多线程并行处理多个数据块，利用CPU多核优势提升吞吐量。同时结合SIMD指令集对T-Table访问和数据变换进行矢量化加速，实现批量数据的并行处理，最大化利用硬件资源，达到更高的加密解密性能。花费时间如下：
![image](https://github.com/user-attachments/assets/e3164531-5574-4062-9370-e225d1659c51)

