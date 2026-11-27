[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/svg-path

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-svg-path` ([https://github.com/jkroso/parse-svg-path](https://github.com/jkroso/parse-svg-path))（原库名 `parse-svg-path` → CP 包名 `@cap/svg-path`）

Zero-allocation SVG 1.1/2.0 `<path d="...">` command parser, 2D bounding-box calculator, and canonical formatter for CP — clean-room MIT parity with npm `parse-svg-path` / `svg-path-parser` and Rust `svgtypes`.

零堆分配的 CP SVG 路径指令词法解析与包围盒计算引擎，完整支持 SVG 全部 20 个命令（`M/m/L/l/H/h/V/v/C/c/S/s/Q/q/T/t/A/a/Z/z`）、紧凑负号（`M10-20`）、无前导零小数（`.5.5`）、科学计数法（`1e-3`）、弧线紧凑标志位（`A25 25 0 1150 25`）、隐式重复命令（`M->L`）与规范化回写（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/svg-path
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `PathCmd` | `pub struct { code: u8, relative: bool, args: [7]f64, arg_count: u8 }` | 单条 SVG 路径指令 |
| `ParsedPath` | `pub struct { cmds: [64]PathCmd, count: usize, valid: bool }` | 栈上最多 64 条指令的解析结果（提供 `cmdAt(i)`） |
| `PathBounds` | `pub struct { valid: bool, min_x, min_y, max_x, max_y, width, height: f64 }` | 二维包围盒结构 |
| `parseSvgPath` | `(d: string) -> ParsedPath` | 词法与语法解析 SVG `d` 路径字符串 |
| `computeBounds` | `(p: ParsedPath) -> PathBounds` | 展开相对坐标与子路径起点，计算 2D 包围盒 |
| `formatSvgPath` | `(p: ParsedPath, out: []u8) -> string` | 将解析后的路径规范化回写为标准 SVG `d` 字符串 |

## Usage / 使用示例

```cp
import { parseSvgPath, computeBounds, formatSvgPath } from "@cap/svg-path";

const path = parseSvgPath("M10-20l30 40H0Z");
const box = computeBounds(path);
// box.min_x == 0, box.min_y == -20, box.max_x == 40, box.max_y == 20, box.width == 40, box.height == 40

let out: [256]u8 = undefined;
const canonical = formatSvgPath(path, out[0..256]); // "M 10 -20 l 30 40 H 0 Z"
```

## License

MIT
