[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`opencv`](https://github.com/opencv/opencv)（`Apache-2.0`）（包名 `@cap/opencv`）。

# `@cap/opencv`

Lean computer vision binding for CP powered by **OpenCV (`4.10.0`, Apache-2.0 License)** — shipping only the stripped `core` + `imgproc` prebuilt dynamic libraries and a thin C-ABI shim for zero local OpenCV build time.

## Architecture

- **Prebuilt Dynamic Libraries (`prebuilt/<os>-<arch>/`)**: Ships stripped `libopencv_core.so*` and `libopencv_imgproc.so*` (preserving `SONAME` symlinks). Excluded from Git tracking (`prebuilt/` is gitignored).
- **Thin C-ABI Shim (`c/opencv_ffi.cpp`)**: ~50-line C++17 bridge compiled into `libopencv_shim.a` in <0.5s, exposing pure C entrypoints for `version`, `GaussianBlur`, `Canny`, and `resize`.
- **Safe CP API (`src/index.cp`)**: Pure CP 4-Gate (`0/0/0/0`) slice-checked wrappers (`version`, `gaussianBlurU8`, `cannyU8`, `resizeU8`).

## Building Minimal OpenCV Prebuilts (Linux x86_64)

```bash
curl -fSL https://github.com/opencv/opencv/archive/refs/tags/4.10.0.tar.gz -o /tmp/opencv-4.10.0.tar.gz
tar -xzf /tmp/opencv-4.10.0.tar.gz -C /tmp
cmake -S /tmp/opencv-4.10.0 -B /tmp/opencv-build -G Ninja \
  -DBUILD_LIST=core,imgproc -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOCS=OFF \
  -DBUILD_opencv_apps=OFF -DWITH_IPP=OFF -DWITH_ITT=OFF -DWITH_OPENCL=OFF \
  -DWITH_CUDA=OFF -DWITH_FFMPEG=OFF -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF \
  -DWITH_QT=OFF -DWITH_VTK=OFF -DWITH_TBB=OFF -DWITH_OPENEXR=OFF \
  -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_JPEG=OFF -DWITH_PNG=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/opencv-build -j$(nproc)
mkdir -p prebuilt/linux-x86_64
cp -a /tmp/opencv-build/lib/libopencv_core.so* /tmp/opencv-build/lib/libopencv_imgproc.so* prebuilt/linux-x86_64/
find prebuilt/linux-x86_64 -maxdepth 1 -type f -name "*.so*" -exec strip --strip-unneeded {} +
```

## Quick Start

```cp
import { version, gaussianBlurU8, cannyU8, resizeU8 } from "@cap/opencv";
import std from "std";

pub fn main(): void! {
    std.debug.print("OpenCV version: {s}\n", .{version()});
    let img: [256]u8 = undefined;
    let blur: [256]u8 = undefined;
    let edges: [256]u8 = undefined;
    let small: [64]u8 = undefined;
    // Fill 16x16 step image (left half 0, right half 255)
    for (y of 0..16) {
        for (x of 0..16) {
            img[y * 16 + x] = if (x < 8) 0 else 255;
        }
    }
    try gaussianBlurU8(&img, &blur, 16, 16, 3);
    try cannyU8(&img, &edges, 16, 16, 50.0, 150.0);
    try resizeU8(&img, 16, 16, &small, 8, 8);
}
```
