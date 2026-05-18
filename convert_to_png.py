#!/usr/bin/env python3

from pathlib import Path
import cairosvg

# Input and output directories
INPUT_DIR = Path("assets")
OUTPUT_DIR = Path("pngs")

# Create output directory if it doesn't exist
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# Convert all SVG files
for svg_file in INPUT_DIR.glob("*.svg"):
    png_file = OUTPUT_DIR / f"{svg_file.stem}.png"

    cairosvg.svg2png(
        url=str(svg_file),
        write_to=str(png_file),
        scale = 4
    )

    print(f"Converted: {svg_file.name} -> {png_file.name}")

print("Done.")