#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path

from render_chimney_video import (
    ROOT,
    OUTPUT_DIR,
    CACHE_DIR,
    parse_float_var,
    parse_int_var,
    glad_include_dirs,
    load_house_cutout,
    apply_night_tint,
    make_night_sky,
    visual_bottom_padding,
    smoke_alpha_byte,
    Image,
)

SNAPSHOT_CPP = CACHE_DIR / "render_chimney_snapshot.cpp"
SNAPSHOT_BIN = CACHE_DIR / "render_chimney_snapshot"
SNAPSHOT_PGM = CACHE_DIR / "snapshot.pgm"
DEFAULT_OUTPUT = OUTPUT_DIR / "house_chimney_snapshot_5s.png"

RENDERER_CPP = r'''
#include "global.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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

static void write_snapshot(const std::filesystem::path &path, int N)
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
        std::cerr << "usage: render_chimney_snapshot <output_pgm> <frame_count> <white_density>\n";
        return 2;
    }

    std::filesystem::path output_pgm = argv[1];
    int frame_count = std::max(1, std::atoi(argv[2]));
    render_white_density = std::max(0.001f, std::stof(argv[3]));

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
    std::cout << "[simulate] start snapshot: " << frame_count << " frames" << std::endl;

    for (int frame = 0; frame < frame_count; frame++)
    {
        std::cout << "[simulate] frame " << (frame + 1) << "/" << frame_count << std::endl;
        update_obstacles(N);
        get_from_UI(N, dens_prev, u_prev, v_prev);
        vel_step(N, u, v, u_prev, v_prev, visc, dt);
        dens_step(N, dens, dens_prev, u, v, diff, dt);
        temp_step(N, temp, temp_prev, u, v, temperature_diff, dt);
        t += dt;
    }

    write_snapshot(output_pgm, N);
    std::cout << "[simulate] done: wrote " << output_pgm << std::endl;
    return 0;
}
'''


def run(cmd: list[str | Path], cwd: Path = ROOT) -> None:
    printable = " ".join(str(part) for part in cmd)
    print(f"+ {printable}", flush=True)
    subprocess.run([str(part) for part in cmd], cwd=cwd, check=True)


def ensure_tools() -> None:
    if shutil.which("g++") is None:
        raise SystemExit("g++ not found. Install g++ or set CXX support first.")
    if Image is None:
        raise SystemExit("Pillow not found. Install pillow to composite the snapshot.")
    if not glad_include_dirs():
        raise SystemExit("glad/glad.h not found. The renderer needs the same GLAD include path as the main build.")


def build_renderer(cxx: str) -> None:
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    SNAPSHOT_CPP.write_text(RENDERER_CPP)

    cmd: list[str | Path] = [
        cxx,
        "-std=c++17",
        "-O2",
        f"-I{ROOT / 'header'}",
    ]

    for include_dir in glad_include_dirs():
        cmd.append(f"-I{include_dir}")

    cmd.extend([
        SNAPSHOT_CPP,
        ROOT / "source" / "global.cpp",
        ROOT / "source" / "simulation.cpp",
        "-o",
        SNAPSHOT_BIN,
    ])
    run(cmd)


def render_snapshot(frame_count: int, white_density: float) -> None:
    SNAPSHOT_PGM.parent.mkdir(parents=True, exist_ok=True)
    run([SNAPSHOT_BIN, SNAPSHOT_PGM, str(frame_count), f"{white_density:.8g}"])


def composite_snapshot(output: Path, output_width: int, output_height: int) -> None:
    assert Image is not None
    print(f"[image] compositing snapshot: {output}", flush=True)

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

    scene = make_night_sky(output_width, output_height)
    scene.alpha_composite(house, (house_x, house_y))

    smoke_alpha = Image.open(SNAPSHOT_PGM).convert("L")
    smoke_alpha = smoke_alpha.resize((output_width, output_height), resampling.BICUBIC)
    smoke_alpha = smoke_alpha.point(smoke_alpha_byte)
    smoke = Image.new("RGBA", (output_width, output_height), (235, 235, 235, 0))
    smoke.putalpha(smoke_alpha)
    scene.alpha_composite(smoke)

    output.parent.mkdir(parents=True, exist_ok=True)
    scene.convert("RGB").save(output)


def main() -> int:
    current_dt = parse_float_var("dt", 0.04)
    current_white_density = parse_float_var("render_white_density", 10.0)
    current_window_width = parse_int_var("SCR_WIDTH", 1000)
    current_window_height = parse_int_var("SCR_HEIGHT", current_window_width)

    parser = argparse.ArgumentParser(description="Render one chimney smoke snapshot without making a video.")
    parser.add_argument("--seconds", type=float, default=5.0, help="simulation time for snapshot; default: 5")
    parser.add_argument("--white-density", type=float, default=current_white_density, help="density value mapped to pure white")
    parser.add_argument("--size", type=int, default=None, help="square output image size in pixels; overrides width/height")
    parser.add_argument("--width", type=int, default=current_window_width, help="output image width in pixels")
    parser.add_argument("--height", type=int, default=current_window_height, help="output image height in pixels")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="PNG output path")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"), help="C++ compiler; default: CXX or g++")
    args = parser.parse_args()

    ensure_tools()

    if current_dt > 0.0:
        frame_count = max(1, int(round(args.seconds / current_dt)))
    else:
        frame_count = max(1, int(round(args.seconds * 25.0)))

    output = args.output if args.output.is_absolute() else ROOT / args.output
    if args.size is not None:
        output_width = max(64, args.size)
        output_height = output_width
    else:
        output_width = max(64, args.width)
        output_height = max(64, args.height)

    print(f"dt={current_dt:g}, snapshot_time={args.seconds:g}s, frames={frame_count}, size={output_width}x{output_height}, white_density={args.white_density:g}", flush=True)
    build_renderer(args.cxx)
    render_snapshot(frame_count, args.white_density)
    composite_snapshot(output, output_width, output_height)

    print(f"saved: {output}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
