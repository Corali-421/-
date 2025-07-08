import cv2
import numpy as np
from scipy.fftpack import dct, idct
import os

def apply_dct(img):
    return dct(dct(img.T, norm='ortho').T, norm='ortho')

def apply_idct(img):
    return idct(idct(img.T, norm='ortho').T, norm='ortho')

def embed_single_channel(channel, wm_resized, alpha):
    dct_cover = apply_dct(np.float32(channel))
    h, w = wm_resized.shape
    block = dct_cover[:h, :w]

    # SVD of watermark
    Uw, Sw, Vw = np.linalg.svd(wm_resized, full_matrices=False)
    # SVD of block
    Ub, Sb, Vb = np.linalg.svd(block, full_matrices=False)

    Se = Sb + alpha * Sw
    embedded_block = np.dot(Ub, np.dot(np.diag(Se), Vb))
    dct_cover[:h, :w] = embedded_block

    return np.clip(apply_idct(dct_cover), 0, 255).astype(np.uint8)

def embed_rgb_watermark(cover_path, watermark_path, output_path='output/watermarked_rgb.png', alpha=0.05):
    cover = cv2.imread(cover_path)
    if cover is None:
        raise FileNotFoundError(f"Cover image {cover_path} not found.")
    watermark = cv2.imread(watermark_path, cv2.IMREAD_GRAYSCALE)
    if watermark is None:
        raise FileNotFoundError(f"Watermark image {watermark_path} not found.")

    wm_resized = cv2.resize(watermark, (cover.shape[1] // 4, cover.shape[0] // 4))
    h, w = wm_resized.shape

    result_channels = []
    for c in cv2.split(cover):
        result_channels.append(embed_single_channel(c, wm_resized, alpha))

    watermarked_rgb = cv2.merge(result_channels)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    cv2.imwrite(output_path, watermarked_rgb)
    print(f"[+] RGB 水印图保存至 {output_path}")

    return h, w, wm_resized

if __name__ == '__main__':
    embed_rgb_watermark('lena.png', 'logo.png')
