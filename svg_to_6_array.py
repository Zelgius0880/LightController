import os
import argparse
import re
import io
import numpy as np
import cairosvg
from PIL import Image
from pathlib import Path

# The target palette (indices 1-6)
# Value 0 is reserved for Transparency
PALETTE = np.array([
    [0, 0, 0],       # 1: Black
    [255, 255, 255], # 2: White
    [255, 243, 56],  # 3: Yellow
    [191, 0, 0],     # 4: Red
    [100, 64, 255],  # 5: Blue/Purple
    [67, 138, 28]    # 6: Green
], dtype=float)

def sanitize_name(name):
    return re.sub(r'[^a-zA-Z0-9_]', '_', name)

def find_nearest_palette_idx(pixel):
    distances = np.sum((PALETTE - pixel)**2, axis=1)
    return np.argmin(distances) + 1

def apply_jjn_dithering(image_np, alpha_mask):
    h, w, _ = image_np.shape
    pixels = image_np.astype(float)
    indexed_output = np.zeros((h, w), dtype=np.uint8)

    kernel = [
        (0, 1, 7/48), (0, 2, 5/48),
        (1, -2, 3/48), (1, -1, 5/48), (1, 0, 7/48), (1, 1, 5/48), (1, 2, 3/48),
        (2, -2, 1/48), (2, -1, 3/48), (2, 0, 5/48), (2, 1, 3/48), (2, 2, 1/48)
    ]

    for y in range(h):
        for x in range(w):
            if not alpha_mask[y, x]:
                indexed_output[y, x] = 0
                continue

            old_pixel = pixels[y, x].copy()
            idx = find_nearest_palette_idx(old_pixel)
            indexed_output[y, x] = idx

            new_pixel = PALETTE[idx - 1]
            error = old_pixel - new_pixel

            for dy, dx, weight in kernel:
                ny, nx = y + dy, x + dx
                if 0 <= ny < h and 0 <= nx < w:
                    pixels[ny, nx] += error * weight

    return indexed_output

def save_preview_image(indexed_array, output_path):
    """Reconstructs an RGBA image from the 0-6 index array for visual verification."""
    h, w = indexed_array.shape
    # Create an empty RGBA buffer (0,0,0,0 is transparent)
    preview_pixels = np.zeros((h, w, 4), dtype=np.uint8)

    for val in range(1, 7):
        mask = (indexed_array == val)
        color = PALETTE[val - 1].astype(np.uint8)
        preview_pixels[mask, 0:3] = color
        preview_pixels[mask, 3] = 255 # Make opaque

    img = Image.fromarray(preview_pixels, 'RGBA')
    img.save(output_path)

def generate_header(data_array, name):
    h, w = data_array.shape
    var_name = sanitize_name(name)
    flat_data = data_array.flatten()

    content = [
        f"#ifndef {var_name.upper()}_H",
        f"#define {var_name.upper()}_H",
        "#include <stdint.h>",
        f"// JJN Dithered Palette: 0=Transp, 1=Blk, 2=Wht, 3=Yel, 4=Red, 5=Blu, 6=Grn",
        f"const uint16_t {var_name}_width = {w};",
        f"const uint16_t {var_name}_height = {h};",
        f"const uint8_t {var_name}_data[{len(flat_data)}] = {{"
    ]

    hex_vals = [f"{v}" for v in flat_data]
    for i in range(0, len(hex_vals), 20):
        line = ", ".join(hex_vals[i:i+20])
        content.append(f"    {line},")

    content.append("};\n#endif")
    return "\n".join(content)

def main():
    parser = argparse.ArgumentParser(description="SVG to JJN Dithered C Header Converter")
    parser.add_argument("input_dir", help="Directory with SVG files")
    parser.add_argument("--out", default="headers", help="C header output directory")
    parser.add_argument("--preview_dir", default="previews", help="PNG preview output directory")
    parser.add_argument("--width", type=int, help="Target pixel width")
    parser.add_argument("--height", type=int, help="Target pixel height")
    parser.add_argument("--preview", action="store_true", help="Generate PNG previews")

    args = parser.parse_args()

    out_path = Path(args.out)
    out_path.mkdir(exist_ok=True)

    if args.preview:
        preview_path = Path(args.preview_dir)
        preview_path.mkdir(exist_ok=True)

    for svg_file in Path(args.input_dir).glob("*.svg"):
        print(f"Processing: {svg_file.name}")
        try:
            # 1. Render and Dither
            png_data = cairosvg.svg2png(url=str(svg_file), output_width=args.width, output_height=args.height)
            img = Image.open(io.BytesIO(png_data)).convert('RGBA')
            img_np = np.array(img)

            alpha_mask = img_np[:, :, 3] > 128
            data = apply_jjn_dithering(img_np[:, :, :3], alpha_mask)

            # 2. Save C Header
            header = generate_header(data, svg_file.stem)
            with open(out_path / f"{sanitize_name(svg_file.stem)}.h", "w") as f:
                f.write(header)

            # 3. Optional Preview
            if args.preview:
                save_preview_image(data, preview_path / f"{svg_file.stem}_preview.png")

            print(f"  [OK] Generated header.")
        except Exception as e:
            print(f"  [ERROR] {e}")

if __name__ == "__main__":
    main()