import cv2
import numpy as np
from scipy.fftpack import dct
import os

def apply_dct(img):
    return dct(dct(img.T, norm='ortho').T, norm='ortho')

def extract_single_channel(orig_channel, wm_channel, Uw, Vw, alpha, h, w):
    dct_orig = apply_dct(np.float32(orig_channel))
    dct_wm = apply_dct(np.float32(wm_channel))

    block_orig = dct_orig[:h, :w]
    block_wm = dct_wm[:h, :w]

    _, So, _ = np.linalg.svd(block_orig, full_matrices=False)
    _, Sw, _ = np.linalg.svd(block_wm, full_matrices=False)

    extracted_S = (Sw - So) / alpha
    wm = np.dot(Uw, np.dot(np.diag(extracted_S), Vw))
    return np.clip(wm, 0, 255).astype(np.uint8)

def extract_rgb_watermark(watermarked_path, original_path, watermark_path, alpha=0.05, output_path='output/extracted.png'):
    watermarked_img = cv2.imread(watermarked_path)
    orig_img = cv2.imread(original_path)
    watermark = cv2.imread(watermark_path, cv2.IMREAD_GRAYSCALE)

    if watermarked_img is None or orig_img is None or watermark is None:
        raise FileNotFoundError("图片路径错误，请检查！")

    # 先缩放水印到嵌入时的尺寸
    wm_emb_size = (watermarked_img.shape[1] // 4, watermarked_img.shape[0] // 4)
    wm_resized = cv2.resize(watermark, wm_emb_size)

    h, w = wm_resized.shape

    # 计算水印的U和V
    Uw, Sw, Vw = np.linalg.svd(wm_resized, full_matrices=False)

    # 只用第一个通道提取水印
    c_orig = orig_img[:, :, 0]
    c_wm = watermarked_img[:, :, 0]

    extracted_small = extract_single_channel(c_orig, c_wm, Uw, Vw, alpha, h, w)

    # 放大提取出来的水印到原始水印大小
    extracted = cv2.resize(extracted_small, (watermark.shape[1], watermark.shape[0]), interpolation=cv2.INTER_CUBIC)

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    cv2.imwrite(output_path, extracted)
    print(f"[+] 提取水印保存至 {output_path}")

if __name__ == '__main__':
    extract_rgb_watermark('output/watermarked_rgb.png', 'lena.png', 'logo.png')
