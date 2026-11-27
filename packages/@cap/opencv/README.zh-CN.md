[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/opencv

> 版本：`4.10.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

OpenCV (v4.10.0) core + imgproc computer vision library for CP via prebuilt dynamic libraries

## 安装与导入

```bash
cap add @cap/opencv
```

```cp
import { cp_cv_version } from "@cap/opencv";
```

## 核心导出 API

- `export fn cp_cv_version(): [*:0]&u8!;`
- `export fn cp_cv_gaussian_blur_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, ksize: i32): i32;`
- `export fn cp_cv_canny_u8(src: [*]&u8!, dst: [*]u8!, w: i32, h: i32, t1: f64, t2: f64): i32;`
- `export fn cp_cv_resize_u8(src: [*]&u8!, sw: i32, sh: i32, dst: [*]u8!, dw: i32, dh: i32): i32;`
- `export fn version(): string;`
- `export fn gaussianBlurU8(src: &[]u8, dst: []u8, w: i32, h: i32, ksize: i32): !void;`
- `export fn cannyU8(src: &[]u8, dst: []u8, w: i32, h: i32, t1: f64, t2: f64): !void;`
- `export fn resizeU8(src: &[]u8, sw: i32, sh: i32, dst: []u8, dw: i32, dh: i32): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`opencv@4.10.0`)，许可证：Apache-2.0
