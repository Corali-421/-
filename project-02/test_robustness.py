import cv2
import numpy as np
import os
from scipy.fftpack import dct, idct

def apply_dct(img):
    return dct(dct(img.T, norm='ortho').T, norm='ortho')

def apply_idct(img):
    return idct(idct(img.T, norm='ortho').T, norm='ortho')

def embed_single_channel(channel, wm_resized, alpha):
    dct_cover = apply_dct(np.float32(channel))
    h, w = wm_resized.shape
    block = dct_cover[:h, :w]

    Uw, Sw, Vw = np.linalg.svd(wm_resized, full_matrices=False)
    Ub, Sb, Vb = np.linalg.svd(block, full_matrices=False)

    Se = Sb + alpha * Sw
    embedded_block = np.dot(Ub, np.dot(np.diag(Se), Vb))
    dct_cover[:h, :w] = embedded_block

    idct_img = apply_idct(dct_cover)
    return np.clip(idct_img, 0, 255).astype(np.uint8)

def embed_rgb_watermark(cover_path, watermark_path, alpha=0.02, output_path='output/watermarked_rgb.png'):
    cover = cv2.imread(cover_path)
    watermark = cv2.imread(watermark_path, cv2.IMREAD_GRAYSCALE)

    if cover is None or watermark is None:
        raise FileNotFoundError("原图或水印图像未找到！")

    # 缩放水印到cover的1/4尺寸
    wm_resized = cv2.resize(watermark, (cover.shape[1] // 4, cover.shape[0] // 4))
    h, w = wm_resized.shape

    result_channels = []
    for c in cv2.split(cover):
        result_channels.append(embed_single_channel(c, wm_resized, alpha))

    watermarked = cv2.merge(result_channels)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    cv2.imwrite(output_path, watermarked)
    print(f"[+] 嵌入水印完成，保存为 {output_path}")

    return h, w, wm_resized, watermark.shape

def extract_single_channel(orig_channel, wm_channel, Uw, Vw, alpha, h, w):
    dct_orig = apply_dct(np.float32(orig_channel))
    dct_wm = apply_dct(np.float32(wm_channel))

    block_orig = dct_orig[:h, :w]
    block_wm = dct_wm[:h, :w]

    _, So, _ = np.linalg.svd(block_orig, full_matrices=False)
    _, Sw, _ = np.linalg.svd(block_wm, full_matrices=False)

    extracted_S = (Sw - So) / alpha
    wm = np.dot(Uw, np.dot(np.diag(extracted_S), Vw))

    # 归一化，映射到0-255
    wm_norm = cv2.normalize(wm, None, 0, 255, cv2.NORM_MINMAX)
    return wm_norm.astype(np.uint8)

def extract_rgb_watermark(watermarked_img, original_img, wm_resized, alpha=0.02, orig_w_h=None):
    h, w = wm_resized.shape
    Uw, Sw, Vw = np.linalg.svd(wm_resized, full_matrices=False)

    extracted_channels = []
    for c_orig, c_wm in zip(cv2.split(original_img), cv2.split(watermarked_img)):
        wm_c = extract_single_channel(c_orig, c_wm, Uw, Vw, alpha, h, w)
        extracted_channels.append(wm_c)

    mean_wm = np.mean(np.stack(extracted_channels, axis=0), axis=0)

    # 如果给了原始水印尺寸，放大到原始大小，保持比例
    if orig_w_h is not None:
        orig_w, orig_h = orig_w_h[1], orig_w_h[0]
        mean_wm = cv2.resize(mean_wm.astype(np.uint8), (orig_w, orig_h), interpolation=cv2.INTER_CUBIC)

    return mean_wm.astype(np.uint8)

# --------- 图像攻击相关函数 ---------

def apply_flip(img):
    return cv2.flip(img, 1)

def apply_shift(img, dx=10, dy=10):
    h, w = img.shape[:2]
    M = np.float32([[1, 0, dx], [0, 1, dy]])
    return cv2.warpAffine(img, M, (w, h))

def apply_crop(img, margin=30):
    return img[margin:-margin, margin:-margin]

def apply_contrast(img, alpha=1.5):
    return cv2.convertScaleAbs(img, alpha=alpha, beta=0)

def apply_noise(img, sigma=10):
    noise = np.random.normal(0, sigma, img.shape).astype(np.uint8)
    return cv2.add(img, noise)

# --------- 主流程 ---------

def run_attacks_and_extract(cover_path, watermark_path, alpha=0.02):
    # 1. 嵌入水印
    h, w, wm_resized, orig_w_h = embed_rgb_watermark(cover_path, watermark_path, alpha=alpha)

    original_img = cv2.imread(cover_path)
    watermarked_img = cv2.imread('output/watermarked_rgb.png')

    attacks = {
        "flip": apply_flip,
        "shift": apply_shift,
        "crop": apply_crop,
        "contrast": apply_contrast,
        "noise": apply_noise,
    }

    attacked_dir = 'output/attacks'
    extracted_dir = 'output/attacks_extracted'
    os.makedirs(attacked_dir, exist_ok=True)
    os.makedirs(extracted_dir, exist_ok=True)

    for name, func in attacks.items():
        print(f"[*] 处理攻击: {name}")
        attacked_img = func(watermarked_img)

        # crop会导致尺寸变小，resize回原尺寸方便提取
        if name == "crop":
            attacked_img = cv2.resize(attacked_img, (watermarked_img.shape[1], watermarked_img.shape[0]))

        attacked_path = os.path.join(attacked_dir, f"{name}.png")
        cv2.imwrite(attacked_path, attacked_img)

        extracted = extract_rgb_watermark(attacked_img, original_img, wm_resized, alpha=alpha, orig_w_h=orig_w_h)

        extracted_path = os.path.join(extracted_dir, f"{name}_extracted.png")
        cv2.imwrite(extracted_path, extracted)
        print(f"[+] 攻击 {name} 的提取水印保存至 {extracted_path}")

if __name__ == '__main__':
    cover_path = 'lena.png'
    watermark_path = 'logo.png'
    run_attacks_and_extract(cover_path, watermark_path)
