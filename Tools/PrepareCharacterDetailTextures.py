"""Turn the CC0 character sources into neutral detail multipliers plus a procedural bristle set.

Run: blender --background --factory-startup --python Tools/PrepareCharacterDetailTextures.py
Run Tools/FetchCharacterDetailTextures.py first. Writes 1024 px PNGs to
Art/Characters/Detail/Generated (ignored by git) for ApplyRiggedMaterials.py.

Detail maps are linear: 0.5 is neutral, so the material multiplies the baked atlas by 2 * Detail.
Normals stay OpenGL; UE flips green once on import.
"""
import json
import math
from pathlib import Path

import bpy
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / "Art/Characters/Detail/sources.json"
OUTPUT = ROOT / "Art/Characters/Detail/Generated"
SIZE = 1024
SEED = 517203
# The atlas owns the authored palette; sources only contribute a hint of hue variation.
SATURATION = .4


def load_pixels(path):
    image = bpy.data.images.load(str(path), check_existing=False)
    image.colorspace_settings.name = "Non-Color"
    # Some sources are a few pixels off (rough_linen is 1024x1026) or elongated (bark is 1024x2048).
    width, height = (SIZE * max(1, round(n / SIZE)) for n in image.size)
    if (width, height) != tuple(image.size):
        image.scale(width, height)
    pixels = np.empty(width * height * 4, np.float32)
    image.pixels.foreach_get(pixels)
    bpy.data.images.remove(image)
    # Box-averaging an elongated tile keeps it seamless; the material assumes square tiles.
    return pixels.reshape(SIZE, height // SIZE, SIZE, width // SIZE, 4).mean(axis=(1, 3))


def save_png(name, rgb):
    image = bpy.data.images.new(name, width=SIZE, height=SIZE, alpha=False)
    image.colorspace_settings.name = "Non-Color"
    rgba = np.concatenate((np.clip(rgb, 0, 1), np.ones((SIZE, SIZE, 1), np.float32)), axis=-1)
    image.pixels.foreach_set(rgba.astype(np.float32).ravel())
    image.filepath_raw = str(OUTPUT / (name + ".png"))
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def neutral_detail(colour):
    """Scale each channel to a 0.5 mean so the multiplier keeps the atlas hue and brightness."""
    linear = np.where(colour <= .04045, colour / 12.92, ((colour + .055) / 1.055) ** 2.4)
    luminance = linear @ np.array((.2126, .7152, .0722), np.float32)
    linear = luminance[..., None] + SATURATION * (linear - luminance[..., None])
    return np.clip(linear / (2 * linear.reshape(-1, 3).mean(axis=0)), 0, 1)


def polyhaven_detail(source):
    maps = {item["map"]: ROOT / item["path"] for item in source["files"]}
    colour = load_pixels(maps["diff"])[..., :3]
    occlusion = load_pixels(maps["arm"])[..., :1]
    name = "T_CD_" + source["asset_id"]
    save_png(name + "_Detail", neutral_detail(colour * occlusion))
    save_png(name + "_Normal", load_pixels(maps["nor_gl"])[..., :3])
    return name


def stamp(field, value, cy, cx, radius):
    """Max-composite a soft disc with toroidal wrap so the map tiles."""
    r = int(math.ceil(radius)) + 1
    offsets = np.arange(-r, r + 1)
    rows = (int(cy) + offsets) % SIZE
    cols = (int(cx) + offsets) % SIZE
    dy = offsets[:, None] + int(cy) - cy
    dx = offsets[None, :] + int(cx) - cx
    disc = np.clip(1 - np.sqrt(dx * dx + dy * dy) / radius, 0, 1) ** .7 * value
    block = field[np.ix_(rows, cols)]
    field[np.ix_(rows, cols)] = np.maximum(block, disc)


def bristle_detail():
    """Coarse overlapping strands that lie along +V, the body-length axis of the triplanar projection."""
    rng = np.random.default_rng(SEED)
    height = np.zeros((SIZE, SIZE), np.float32)
    shade = np.full((SIZE, SIZE), .5, np.float32)
    for layer in range(3):
        for _ in range(1500):
            x, y = rng.random(2) * SIZE
            heading = math.pi / 2 + rng.normal(0, .22)
            length = rng.uniform(60, 120)
            width = rng.uniform(2.2, 3.6)
            level = .45 + .25 * layer + rng.uniform(0, .2)
            tip = rng.uniform(.55, .9)
            steps = int(length / 1.5)
            bend = rng.normal(0, .004)
            for step in range(steps):
                t = step / steps
                heading += bend
                x += math.cos(heading) * 1.5
                y += math.sin(heading) * 1.5
                stamp(height, level * (1 - .35 * t), y, x, width * (1 - .55 * t))
                stamp(shade, .45 + tip * t * .4, y, x, width * .8 * (1 - .55 * t))
    y, x = np.mgrid[0:SIZE, 0:SIZE].astype(np.float32) / SIZE
    clumps = np.zeros_like(x)
    for a, b in rng.integers(1, 6, (6, 2)):
        clumps += np.sin(math.tau * (a * x + b * y) + rng.random() * math.tau)
    shade = shade * (.85 + .05 * clumps) * (.7 + .5 * height)
    detail = np.repeat((shade / (2 * shade.mean()))[..., None], 3, axis=-1)
    dx = np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)
    dy = np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)
    normal = np.stack((-dx * 6, -dy * 6, np.ones_like(height)), axis=-1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    save_png("T_CD_bristle_Detail", detail)
    save_png("T_CD_bristle_Normal", normal * .5 + .5)
    return "T_CD_bristle"


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sources = json.loads(RECORD.read_text(encoding="utf-8"))["sources"]
    names = [polyhaven_detail(source) for source in sources] + [bristle_detail()]
    print("CHARACTER_DETAIL_PREPARED", json.dumps(names))


main()
