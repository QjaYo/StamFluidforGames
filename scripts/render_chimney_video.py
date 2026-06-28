#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import random
import re
import shutil
import subprocess
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    Image = None
    ImageDraw = None
    ImageFilter = None

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "output"
CACHE_DIR = OUTPUT_DIR / ".render_cache"
FRAME_DIR = CACHE_DIR / "frames"
SCENE_FRAME_DIR = CACHE_DIR / "scene_frames"
RENDER_CPP = CACHE_DIR / "render_chimney.cpp"
RENDER_BIN = CACHE_DIR / "render_chimney"
DEFAULT_OUTPUT = OUTPUT_DIR / "chimney_5s.mp4"

RENDERER_CPP = r'''
#include "global.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

void init();
void update_obstacles(int N);
void get_from_UI(int N, float *d, float *u, float *v);
void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt);
void dens_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt);
void temp_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt);

static unsigned char density_to_byte(float d)
{
    if (d < 0.0f)
        d = 0.0f;

    float white_density = render_white_density > 0.0f ? render_white_density : 1.0f;
    d /= white_density;

    if (d > 1.0f)
        d = 1.0f;

    return (unsigned char)std::lround(d * 255.0f);
}

static void write_frame(const std::filesystem::path &path, int N)
{
    std::vector<unsigned char> img((size_t)N * (size_t)N);

    for (int row = 0; row < N; row++)
    {
        int j = N - row;
        for (int col = 0; col < N; col++)
        {
            int i = col + 1;
            img[(size_t)row * (size_t)N + (size_t)col] = density_to_byte(dens[IX(i, j)]);
        }
    }

    std::ofstream out(path, std::ios::binary);
    out << "P5\n" << N << " " << N << "\n255\n";
    out.write(reinterpret_cast<const char *>(img.data()), (std::streamsize)img.size());
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "usage: render_chimney <frame_dir> <frame_count> <white_density>\n";
        return 2;
    }

    std::filesystem::path frame_dir = argv[1];
    int frame_count = std::max(1, std::atoi(argv[2]));
    render_white_density = std::max(0.001f, std::stof(argv[3]));

    std::filesystem::remove_all(frame_dir);
    std::filesystem::create_directories(frame_dir);

    obstacles_enabled = false;
    static_obstacle_enabled = false;
    mouse_obstacle_enabled = false;
    chimney_enabled = true;
    house_scene_enabled = true;
    buoyancy_enabled = true;
    breeze_enabled = true;
    vorticity_enabled = true;
    mouse_down = false;
    obstacle_mouse_down = false;
    boundary_mode = BOUNDARY_SOLID;

    init();

    const int N = SCR_SIZE;

    std::cout << "[simulate] start: " << frame_count << " frames" << std::endl;

    for (int frame = 0; frame < frame_count; frame++)
    {
        std::cout << "[simulate] frame " << (frame + 1) << "/" << frame_count << std::endl;
        update_obstacles(N);
        get_from_UI(N, dens_prev, u_prev, v_prev);
        vel_step(N, u, v, u_prev, v_prev, visc, dt);
        dens_step(N, dens, dens_prev, u, v, diff, dt);
        temp_step(N, temp, temp_prev, u, v, temperature_diff, dt);

        std::ostringstream name;
        name << "frame_" << std::setw(4) << std::setfill('0') << frame << ".pgm";
        write_frame(frame_dir / name.str(), N);

        t += dt;
    }

    std::cout << "[simulate] done: rendered " << frame_count << " frames to " << frame_dir << std::endl;
    return 0;
}
'''


def run(cmd: list[str | Path], cwd: Path = ROOT) -> None:
    printable = " ".join(str(part) for part in cmd)
    print(f"+ {printable}", flush=True)
    subprocess.run([str(part) for part in cmd], cwd=cwd, check=True)


def parse_float_var(name: str, default: float) -> float:
    source = ROOT / "source" / "global.cpp"
    text = source.read_text()
    match = re.search(rf"float\s+{re.escape(name)}\s*=\s*([^;]+);", text)
    if not match:
        return default

    raw = match.group(1).replace("f", "").replace("F", "")
    if not re.fullmatch(r"[0-9eE+\-*/().\s]+", raw):
        return default

    try:
        return float(eval(raw, {"__builtins__": {}}, {}))
    except (SyntaxError, ValueError, ZeroDivisionError):
        return default


def parse_int_var(name: str, default: int) -> int:
    source = ROOT / "source" / "global.cpp"
    text = source.read_text()
    match = re.search(rf"(?:const\s+)?(?:unsigned\s+)?int\s+{re.escape(name)}\s*=\s*([0-9]+)", text)
    if not match:
        return default
    return int(match.group(1))


def glad_include_dirs() -> list[Path]:
    candidates = [
        ROOT / "../vcpkg/packages/glad_x64-linux/include",
        ROOT / "../External/Libraries/include",
        ROOT / "externel",
    ]
    return [path.resolve() for path in candidates if (path / "glad" / "glad.h").exists()]


def ensure_tools() -> None:
    if shutil.which("g++") is None:
        raise SystemExit("g++ not found. Install g++ or set CXX support first.")
    if shutil.which("ffmpeg") is None:
        raise SystemExit("ffmpeg not found. Install ffmpeg to encode mp4.")
    if Image is None or ImageDraw is None or ImageFilter is None:
        raise SystemExit("Pillow not found. Install pillow to composite the house preview frames.")
    if not glad_include_dirs():
        raise SystemExit("glad/glad.h not found. The renderer needs the same GLAD include path as the main build.")


def build_renderer(cxx: str) -> None:
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    RENDER_CPP.write_text(RENDERER_CPP)

    cmd: list[str | Path] = [
        cxx,
        "-std=c++17",
        "-O2",
        f"-I{ROOT / 'header'}",
    ]

    for include_dir in glad_include_dirs():
        cmd.append(f"-I{include_dir}")

    cmd.extend([
        RENDER_CPP,
        ROOT / "source" / "global.cpp",
        ROOT / "source" / "simulation.cpp",
        "-o",
        RENDER_BIN,
    ])
    run(cmd)


def render_frames(frame_count: int, white_density: float) -> None:
    run([RENDER_BIN, FRAME_DIR, str(frame_count), f"{white_density:.8g}"])


def house_background_pixel(pixel: tuple[int, int, int, int]) -> bool:
    r, g, b, _ = pixel
    max_c = max(r, g, b)
    min_c = min(r, g, b)
    return max_c > 215 and max_c - min_c < 42


def smoke_alpha_byte(value: int) -> int:
    den = value / 255.0
    t = max(0.0, min(1.0, (den - 0.035) / (0.95 - 0.035)))
    alpha = t * t * (3.0 - 2.0 * t)
    alpha = pow(alpha, 0.82) * 0.90
    return int(max(0.0, min(1.0, alpha)) * 255.0 + 0.5)


def apply_night_tint(image: Image.Image) -> Image.Image:
    image = image.copy()
    pixels = []
    for r, g, b, a in image.get_flattened_data() if hasattr(image, "get_flattened_data") else image.getdata():
        pixels.append((int(r * 0.22), int(g * 0.24), int(b * 0.34), a))
    image.putdata(pixels)
    return image


def visual_bottom_padding(image: Image.Image) -> int:
    bbox = image.getbbox()
    if not bbox:
        return 0

    alpha = image.getchannel("A")
    width, height = image.size
    min_opaque = max(20, width // 6)
    for y in range(height - 1, -1, -1):
        opaque = sum(1 for x in range(width) if alpha.getpixel((x, y)) > 0)
        if opaque >= min_opaque:
            return height - (y + 1)

    return height - bbox[3]


def add_night_clouds(sky: Image.Image, rng: random.Random) -> Image.Image:
    assert ImageDraw is not None and ImageFilter is not None
    width, height = sky.size
    clouds = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(clouds, "RGBA")

    clusters = [
        (0.18, 0.55, 0.34, 0.12, 28),
        (0.50, 0.47, 0.44, 0.13, 24),
        (0.80, 0.60, 0.38, 0.12, 22),
        (0.35, 0.72, 0.30, 0.10, 18),
    ]

    for cx, cy, sx, sy, alpha in clusters:
        for _ in range(18):
            ox = rng.uniform(-0.48, 0.48) * sx * width
            oy = rng.uniform(-0.55, 0.55) * sy * height
            rx = rng.uniform(0.08, 0.18) * sx * width
            ry = rng.uniform(0.20, 0.46) * sy * height
            x = cx * width + ox
            y = cy * height + oy
            tint = rng.randint(-8, 12)
            color = (max(0, 74 + tint), max(0, 84 + tint), max(0, 116 + tint), alpha + rng.randint(-5, 8))
            draw.ellipse((x - rx, y - ry, x + rx, y + ry), fill=color)

    clouds = clouds.filter(ImageFilter.GaussianBlur(max(3, int(min(width, height) * 0.018))))
    return Image.alpha_composite(sky, clouds)


def make_night_sky(width: int, height: int | None = None) -> Image.Image:
    assert Image is not None and ImageDraw is not None and ImageFilter is not None
    if height is None:
        height = width

    sky = Image.new("RGBA", (width, height), (0, 0, 0, 255))
    draw = ImageDraw.Draw(sky, "RGBA")
    horizon = (6, 8, 20)
    zenith = (1, 2, 9)

    denom = max(1, height - 1)
    for y in range(height):
        uv_y = 1.0 - y / denom
        t = uv_y * uv_y * (3.0 - 2.0 * uv_y)
        color = tuple(int(horizon[c] + (zenith[c] - horizon[c]) * t + 0.5) for c in range(3))
        draw.line((0, y, width, y), fill=(*color, 255))

    rng = random.Random(20260629)
    star_count = max(150, int(width * height / 2500))
    drawn_stars = 0
    attempts = 0
    while drawn_stars < star_count and attempts < star_count * 5:
        attempts += 1
        x = rng.randrange(0, width)
        y = rng.randrange(max(1, int(height * 0.025)), max(2, int(height * 0.90)))
        uv_y = 1.0 - y / max(1, height - 1)
        horizon_fade = max(0.0, min(1.0, (uv_y - 0.045) / 0.16))
        if rng.random() > horizon_fade:
            continue

        brightness = rng.randint(90, 232)
        warm = rng.random()
        color = (
            min(255, int(brightness * (0.78 + 0.25 * warm))),
            min(255, int(brightness * (0.86 + 0.12 * warm))),
            min(255, int(brightness * (1.04 - 0.14 * warm))),
            int(rng.randint(120, 230) * horizon_fade),
        )
        radius = 1 if rng.random() < 0.92 else 2
        if radius == 1:
            draw.point((x, y), fill=color)
        else:
            draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=color)
            soft = (color[0], color[1], color[2], max(25, color[3] // 3))
            draw.line((x - 3, y, x + 3, y), fill=soft)
            draw.line((x, y - 3, x, y + 3), fill=soft)
        drawn_stars += 1

    sky = add_night_clouds(sky, rng)

    moon = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    moon_draw = ImageDraw.Draw(moon, "RGBA")
    cx = int(width * 0.78)
    cy = int(height * 0.21)
    r = max(12, int(min(width, height) * 0.052))

    glow = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow, "RGBA")
    for scale, alpha in ((5.2, 16), (3.2, 25), (1.8, 34)):
        gr = int(r * scale)
        glow_draw.ellipse((cx - gr, cy - gr, cx + gr, cy + gr), fill=(82, 92, 140, alpha))
    sky = Image.alpha_composite(sky, glow.filter(ImageFilter.GaussianBlur(max(1, int(r * 0.55)))))

    moon_draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(224, 215, 174, 255))

    return Image.alpha_composite(sky, moon)


def load_house_cutout() -> Image.Image:
    assert Image is not None
    image = Image.open(ROOT / "asset" / "house.png").convert("RGBA")
    width, height = image.size
    pixels = list(image.get_flattened_data() if hasattr(image, "get_flattened_data") else image.getdata())
    transparent = bytearray(width * height)
    stack: list[int] = []

    def try_push(x: int, y: int) -> None:
        if x < 0 or x >= width or y < 0 or y >= height:
            return
        idx = y * width + x
        if transparent[idx]:
            return
        if not house_background_pixel(pixels[idx]):
            return
        transparent[idx] = 1
        stack.append(idx)

    for x in range(width):
        try_push(x, 0)
        try_push(x, height - 1)
    for y in range(height):
        try_push(0, y)
        try_push(width - 1, y)

    while stack:
        idx = stack.pop()
        x = idx % width
        y = idx // width
        try_push(x + 1, y)
        try_push(x - 1, y)
        try_push(x, y + 1)
        try_push(x, y - 1)

    for idx, clear in enumerate(transparent):
        if clear:
            r, g, b, _ = pixels[idx]
            pixels[idx] = (r, g, b, 0)

    image.putdata(pixels)
    return image


def composite_scene_frames(frame_count: int, output_width: int, output_height: int) -> None:
    print(f"[video] compositing scene frames: {frame_count} frames", flush=True)
    assert Image is not None
    shutil.rmtree(SCENE_FRAME_DIR, ignore_errors=True)
    SCENE_FRAME_DIR.mkdir(parents=True, exist_ok=True)

    resampling = Image.Resampling if hasattr(Image, "Resampling") else Image
    scene_height = max(0.05, min(0.95, parse_float_var("house_scene_height", 1.0 / 3.0)))
    house = apply_night_tint(load_house_cutout())
    house_height = max(1, int(round(output_height * scene_height)))
    house_width = max(1, int(round(house_height * house.size[0] / house.size[1])))
    bottom_padding = visual_bottom_padding(house)
    house_scale = house_height / house.size[1]
    house = house.resize((house_width, house_height), resampling.LANCZOS)
    house_x = (output_width - house_width) // 2
    house_y = output_height - house_height + int(round(bottom_padding * house_scale))
    sky = make_night_sky(output_width, output_height)

    for frame in range(frame_count):
        smoke_path = FRAME_DIR / f"frame_{frame:04d}.pgm"
        scene = sky.copy()
        scene.alpha_composite(house, (house_x, house_y))

        smoke_alpha = Image.open(smoke_path).convert("L")
        smoke_alpha = smoke_alpha.resize((output_width, output_height), resampling.BICUBIC)
        smoke_alpha = smoke_alpha.point(smoke_alpha_byte)
        smoke = Image.new("RGBA", (output_width, output_height), (235, 235, 235, 0))
        smoke.putalpha(smoke_alpha)
        scene.alpha_composite(smoke)

        scene.convert("RGB").save(SCENE_FRAME_DIR / f"scene_{frame:04d}.png")


def encode_video(output: Path, fps: float) -> None:
    print(f"[video] generating mp4: {output}", flush=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    run([
        "ffmpeg",
        "-y",
        "-hide_banner",
        "-loglevel",
        "warning",
        "-framerate",
        f"{fps:.8g}",
        "-start_number",
        "0",
        "-i",
        SCENE_FRAME_DIR / "scene_%04d.png",
        "-vf",
        "format=yuv420p",
        "-movflags",
        "+faststart",
        output,
    ])


def main() -> int:
    current_dt = parse_float_var("dt", 0.04)
    current_white_density = parse_float_var("render_white_density", 10.0)
    current_window_width = parse_int_var("SCR_WIDTH", 900)
    current_window_height = parse_int_var("SCR_HEIGHT", current_window_width)
    default_fps = 1.0 / current_dt if current_dt > 0.0 else 25.0

    parser = argparse.ArgumentParser(description="Render the chimney smoke simulation to an mp4 without opening a window.")
    parser.add_argument("--seconds", type=float, default=5.0, help="simulation seconds to render; default: 5")
    parser.add_argument("--fps", type=float, default=default_fps, help="output fps; default: 1 / dt from source/global.cpp")
    parser.add_argument("--white-density", type=float, default=current_white_density, help="density value mapped to pure white")
    parser.add_argument("--size", type=int, default=None, help="square output video size in pixels; overrides width/height")
    parser.add_argument("--width", type=int, default=current_window_width, help="output video width in pixels")
    parser.add_argument("--height", type=int, default=current_window_height, help="output video height in pixels")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="mp4 output path")
    parser.add_argument("--keep-frames", action="store_true", help="keep intermediate frames in output/.render_cache")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"), help="C++ compiler; default: CXX or g++")
    args = parser.parse_args()

    ensure_tools()

    if current_dt > 0.0:
        frame_count = max(1, int(round(args.seconds / current_dt)))
    else:
        frame_count = max(1, int(round(args.seconds * args.fps)))

    output = args.output if args.output.is_absolute() else ROOT / args.output
    if args.size is not None:
        output_width = max(64, args.size)
        output_height = output_width
    else:
        output_width = max(64, args.width)
        output_height = max(64, args.height)

    print(f"dt={current_dt:g}, frames={frame_count}, fps={args.fps:g}, size={output_width}x{output_height}, white_density={args.white_density:g}", flush=True)
    build_renderer(args.cxx)
    render_frames(frame_count, args.white_density)
    composite_scene_frames(frame_count, output_width, output_height)
    encode_video(output, args.fps)

    if not args.keep_frames:
        shutil.rmtree(FRAME_DIR, ignore_errors=True)
        shutil.rmtree(SCENE_FRAME_DIR, ignore_errors=True)

    print(f"saved: {output}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
