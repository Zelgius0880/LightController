import os
from PIL import Image

INPUT_FOLDER = "icons_png"
OUTPUT_FOLDER = "icons_h"

def rgb_to_brg565(r, g, b):
    # Matches your BRG setup: Blue(5), Red(6), Green(5)
    b5 = (b >> 3) & 0x1F
    r6 = (r >> 2) & 0x3F
    g5 = (g >> 3) & 0x1F
    return (b5 << 11) | (r6 << 5) | g5

def convert_image(filepath, filename):
    img = Image.open(filepath).convert("RGBA")
    width, height = img.size
    name = os.path.splitext(filename)[0].replace("@2x", "").replace("-", "_")

    output_path = os.path.join(OUTPUT_FOLDER, f"{name}.h")

    with open(output_path, "w") as f:
        f.write(f"// Generated for BRG565 + Alpha - {filename}\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define ICON_{name.upper()}_WIDTH {width}\n")
        f.write(f"#define ICON_{name.upper()}_HEIGHT {height}\n\n")

        colors = []
        alphas = []

        pixels = img.load()
        for y in range(height):
            for x in range(width):
                r, g, b, a = pixels[x, y]
                colors.append(f"0x{rgb_to_brg565(r, g, b):04X}")
                alphas.append(f"0x{a:02X}")

        # Write Color Array
        f.write(f"const uint16_t icon_{name}_colors[] PROGMEM = {{\n    ")
        f.write(", ".join(colors))
        f.write("\n};\n\n")

        # Write Alpha Array
        f.write(f"const uint8_t icon_{name}_alpha[] PROGMEM = {{\n    ")
        f.write(", ".join(alphas))
        f.write("\n};\n")

if __name__ == "__main__":
    for folder in [INPUT_FOLDER, OUTPUT_FOLDER]:
        if not os.path.exists(folder): os.makedirs(folder)
    for f in os.listdir(INPUT_FOLDER):
        if f.lower().endswith(".png"): convert_image(os.path.join(INPUT_FOLDER, f), f)