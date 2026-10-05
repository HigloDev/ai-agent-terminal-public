"""Prepare an optional wallpaper for installation with set-wallpaper.ps1."""
import argparse
from pathlib import Path
from PIL import Image,ImageOps
p=argparse.ArgumentParser();p.add_argument('image');a=p.parse_args();source=Path(a.image)
if source.stat().st_size>30*1024*1024:raise SystemExit('图片不能超过 30 MB')
root=Path(__file__).resolve().parent/'.local';root.mkdir(exist_ok=True)
with Image.open(source) as im: ImageOps.fit(ImageOps.exif_transpose(im).convert('RGB'),(1280,720)).save(root/'wallpaper.bmp')
print(root/'wallpaper.bmp')
