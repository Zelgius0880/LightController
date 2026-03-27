import os
import argparse
import re
import io
import numpy as np
import cairosvg
from PIL import Image
from pathlib import Path
import xml.etree.ElementTree as ET

def sanitize_name(name):
    """Converts filename to a valid C variable name."""
    return re.sub(r'[^a-zA-Z0-9_]', '_', name)


def get_boolean_array(svg_path, target_width=None, target_height=None, threshold=128):
    """
    Renders SVG to a boolean array with optional scaling.
    Scaling at the SVG level prevents 'chunkiness' from upscaling bitmaps.
    """
    # Render SVG to PNG in memory with specific dimensions
    # cairosvg handles aspect ratio automatically if only one dimension is provided

    tree = ET.parse(svg_path)
    root = tree.getroot()

    # 2. Modify the color attributes
    # The 'fill' attribute is commonly used for solid colors
    # Ensure you are using the correct namespace (ns) if your SVG has one
    for element in root.iter('{http://www.w3.org/2000/svg}path'):
        element.set('fill', "#ffffff")

    # You might also want to change 'stroke' for outlines, etc.
    for element in root.iter('{http://www.w3.org/2000/svg}circle'):
        element.set('stroke', "#ffffff")

    # Convert the modified XML back to a bytestring
    modified_svg_bytestring = ET.tostring(root, encoding='utf-8')

    png_data = cairosvg.svg2png(
        bytestring=modified_svg_bytestring,
        output_width=target_width,
        output_height=target_height
    )

    image = Image.open(io.BytesIO(png_data)).convert('L')
    img_array = np.array(image)

    # Logic: 1 (True) if pixel is darker than threshold (foreground)
    return img_array < threshold


def generate_header_content(bool_array, name, mode):
    """Creates the string content for the .h file with row-based wrapping."""
    height, width = bool_array.shape
    var_name = sanitize_name(name)

    header = [
        f"#ifndef {var_name.upper()}_H",
        f"#define {var_name.upper()}_H",
        "",
        "#include <stdint.h>",
        "",
        f"// Dimensions: {width}x{height}",
        f"const uint16_t {var_name}_width = {width};",
        f"const uint16_t {var_name}_height = {height};",
        ""
    ]

    rows_hex = []

    if mode == 'packed':
        # Pack each row individually to ensure row alignment
        for row in bool_array:
            packed_row = np.packbits(row, bitorder='big')
            row_str = ", ".join([f"0x{b:02x}" for b in packed_row])
            rows_hex.append(row_str)

        total_elements = len(np.packbits(bool_array, axis=1))  # Total bytes
        header.append(f"const uint8_t {var_name}_data[{total_elements}] = {{")

    else:
        # Unpacked: Each boolean is one byte
        for row in bool_array:
            row_str = ", ".join([f"0x{int(b):02x}" for b in row])
            rows_hex.append(row_str)

        header.append(f"const uint8_t {var_name}_data[{bool_array.size}] = {{")

    # Join rows with newlines and indentation
    header.append("    " + ",\n    ".join(rows_hex))
    header.append("};")
    header.append("")
    header.append(f"#endif // {var_name.upper()}_H")

    return "\n".join(header)


def main():
    parser = argparse.ArgumentParser(description="Convert SVG directory to C header bitmaps with scaling.")
    parser.add_argument("input_dir", help="Directory containing .svg files")
    parser.add_argument("--mode", choices=['packed', 'simple'], default='packed',
                        help="Storage: 'packed' (1 bit/px) or 'simple' (1 byte/px)")
    parser.add_argument("--out", default="headers", help="Output directory")
    parser.add_argument("--width", type=int, help="Target width in pixels")
    parser.add_argument("--height", type=int, help="Target height in pixels")

    args = parser.parse_args()

    input_path = Path(args.input_dir)
    output_path = Path(args.out)
    output_path.mkdir(exist_ok=True)

    if not input_path.is_dir():
        print(f"Error: {args.input_dir} is not a directory.")
        return

    svg_files = list(input_path.glob("*.svg"))

    print(f"Processing {len(svg_files)} files...")
    if args.width or args.height:
        print(f"Target size: {args.width or 'auto'}x{args.height or 'auto'}")

    for svg_file in svg_files:
        try:
            # Pass the new width/height params here
            bool_grid = get_boolean_array(svg_file, args.width, args.height)

            header_text = generate_header_content(bool_grid, svg_file.stem, args.mode)
            header_filename = output_path / f"{sanitize_name(svg_file.stem)}.h"

            with open(header_filename, "w") as f:
                f.write(header_text)

            print(f"  [+] {header_filename.name} ({bool_grid.shape[1]}x{bool_grid.shape[0]})")
        except Exception as e:
            print(f"  [!] Failed {svg_file.name}: {e}")


if __name__ == "__main__":
    main()