import numpy as np
from PIL import Image

def generate_gamut_picker(width=600, height=600, gamut_type='A'):
    # Define Gamut Triangles from your Kotlin Enum
    GAMUTS = {
        'C': {'r': (0.6915, 0.3038), 'g': (0.17, 0.7), 'b': (0.1532, 0.0475)},
        'B': {'r': (0.675, 0.322), 'g': (0.409, 0.518), 'b': (0.167, 0.04)},
        'A': {'r': (0.704, 0.296), 'g': (0.2151, 0.7106), 'b': (0.138, 0.08)}
    }

    tri = GAMUTS[gamut_type]
    r_tri = np.array([tri['r'][0], tri['r'][1]])
    g_tri = np.array([tri['g'][0], tri['g'][1]])
    b_tri = np.array([tri['b'][0], tri['b'][1]])

    # 1. Setup Grid
    y_coords, x_coords = np.mgrid[0:height, 0:width].astype(float)
    cx, cy = width / 2.0, height / 2.0
    radius = min(width, height) / 2.0

    dx = x_coords - cx
    dy = y_coords - cy
    dist = np.sqrt(dx**2 + dy**2)

    # Create Alpha mask (circular picker)
    alpha = np.where(dist <= radius, 255, 0).astype(np.uint8)

    # 2. HSV Calculation
    angle = np.arctan2(dy, dx) # -PI to PI
    hue = (angle + np.pi) / (2 * np.pi)
    sat = np.clip(dist / radius, 0, 1)
    val = 1.0

    # 3. HSV to RGB (Manual implementation to match your logic exactly)
    def hsv_to_rgb_vec(h, s, v):
        i = (h * 6).astype(int)
        f = h * 6 - i
        p = v * (1.0 - s)
        q = v * (1.0 - f * s)
        t = v * (1.0 - (1.0 - f) * s)

        r = np.select([i%6==0, i%6==1, i%6==2, i%6==3, i%6==4, i%6==5], [v, q, p, p, t, v])
        g = np.select([i%6==0, i%6==1, i%6==2, i%6==3, i%6==4, i%6==5], [t, v, v, q, p, p])
        b = np.select([i%6==0, i%6==1, i%6==2, i%6==3, i%6==4, i%6==5], [p, p, t, v, v, q])
        return r, g, b

    r_raw, g_raw, b_raw = hsv_to_rgb_vec(hue, sat, val)

    # 4. RGB to XY (Linearization + Matrix)
    def linearize(c):
        return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055)**2.4)

    r_lin, g_lin, b_lin = linearize(r_raw), linearize(g_raw), linearize(b_raw)

    X = r_lin * 0.4124 + g_lin * 0.3576 + b_lin * 0.1805
    Y_xyz = r_lin * 0.2126 + g_lin * 0.7152 + b_lin * 0.0722
    Z = r_lin * 0.0193 + g_lin * 0.1192 + b_lin * 0.9505

    xyz_sum = X + Y_xyz + Z
    # Avoid division by zero
    safe_sum = np.where(xyz_sum == 0, 1e-9, xyz_sum)
    curr_x = X / safe_sum
    curr_y = Y_xyz / safe_sum

    # 5. Triangle Clamping Logic
    def cross_2d(a, b):
        return a[..., 0] * b[..., 1] - a[..., 1] * b[..., 0]

    def is_inside(p, a, b, c):
        area = cross_2d(b - a, c - a)
        w1 = cross_2d(b - p, c - p) / area
        w2 = cross_2d(c - p, a - p) / area
        w3 = cross_2d(a - p, b - p) / area
        return (w1 >= 0) & (w2 >= 0) & (w3 >= 0)

    def closest_on_seg(p, a, b):
        ab = b - a
        t = np.sum((p - a) * ab, axis=-1) / np.sum(ab * ab)
        t = np.clip(t, 0, 1)
        return a + ab * t[..., np.newaxis]

    p = np.stack([curr_x, curr_y], axis=-1)
    inside = is_inside(p, r_tri, g_tri, b_tri)

    # Calculate projections for all points (vectorized)
    pRG = closest_on_seg(p, r_tri, g_tri)
    pGB = closest_on_seg(p, g_tri, b_tri)
    pBR = closest_on_seg(p, b_tri, r_tri)

    # Find which projection is closest
    dRG = np.linalg.norm(p - pRG, axis=-1)
    dGB = np.linalg.norm(p - pGB, axis=-1)
    dBR = np.linalg.norm(p - pBR, axis=-1)

    min_dist = np.minimum(dRG, np.minimum(dGB, dBR))

    clamped_x = np.select(
        [inside, min_dist == dRG, min_dist == dGB],
        [curr_x, pRG[..., 0], pGB[..., 0]],
        default=pBR[..., 0]
    )
    clamped_y = np.select(
        [inside, min_dist == dRG, min_dist == dGB],
        [curr_y, pRG[..., 1], pGB[..., 1]],
        default=pBR[..., 1]
    )

    # 6. XY to RGB
    # Using Y = 1.0 as per your Kotlin code
    Y_final = 1.0
    # Avoid y=0 division
    clamped_y_safe = np.where(clamped_y == 0, 1e-9, clamped_y)
    X_final = (Y_final / clamped_y_safe) * clamped_x
    Z_final = (Y_final / clamped_y_safe) * (1.0 - clamped_x - clamped_y)

    r_res = X_final * 3.2406 + Y_final * -1.5372 + Z_final * -0.4986
    g_res = X_final * -0.9689 + Y_final * 1.8758 + Z_final * 0.0415
    b_res = X_final * 0.0557 + Y_final * -0.2040 + Z_final * 1.0570

    def gamma(c):
        return np.where(c <= 0.0031308, 12.92 * c, 1.055 * np.power(np.maximum(c, 0), 1/2.4) - 0.055)

    final_r = (np.clip(gamma(r_res), 0, 1) * 255).astype(np.uint8)
    final_g = (np.clip(gamma(g_res), 0, 1) * 255).astype(np.uint8)
    final_b = (np.clip(gamma(b_res), 0, 1) * 255).astype(np.uint8)

    # 7. Construct Image
    img_data = np.stack([final_r, final_g, final_b, alpha], axis=-1)
    img = Image.fromarray(img_data, 'RGBA')
    img.save(f"gamut_{gamut_type}_picker.png")
    print(f"Image saved: gamut_{gamut_type}_picker.png")

if __name__ == "__main__":
    generate_gamut_picker(600, 600, 'C')