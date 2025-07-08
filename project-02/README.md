1.embed_watermark.py 实现了基于DCT（离散余弦变换）+ SVD（奇异值分解）的彩色图像水印嵌入算法。原理如下：  
  对原图每个通道进行 DCT得到频域系数矩阵 C_dct  
  选取 DCT 左上角低频区域 C_block，对 C_block 进行奇异值分解（SVD）：  
  C_block = U_C · S_C · V_C.T  
  对水印图像 W 同样做 SVD：  
  W = U_W · S_W · V_W.T  
  将水印奇异值 S_W 加入到原图奇异值中，嵌入水印：  
  S_E = S_C + α · S_W
  其中 α 控制嵌入强度，越大水印越明显，但可能影响图像质量。  
  重构修改后的频域块：  
  C_block' = U_C · S_E · V_C.T
  将修改后的块放回原 DCT 图，做逆 DCT 得到带水印图：  
  C_watermarked = IDCT(C_dct')  
  最终输出嵌入水印后的彩色图像。  
  lena.png 是被嵌入水印的示例图片；logo.png 是要嵌入的水印图片，这里选择了山大的logo  
  嵌入后的图片存储在output文件夹下，被命名为watermarked_rgb.png
2.extract_watermark.py 实现了图像水印提取。
  并对图像进行了左右翻转，平移（整体向右下角平移10个像素），裁剪（上下左右各裁剪30个像素，截取中间区域），调整对比度（将图片对比度放大1.5倍），添加噪声
