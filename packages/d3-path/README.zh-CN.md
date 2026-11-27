[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `d3-path`

> **源包溯源**：`npm:d3-path` ([d3/d3-path](https://github.com/d3/d3-path)) — 协议：MIT（上游：ISC）

纯 CP 实现的零堆分配 Canvas-to-SVG 路径字符串序列化器，支持 `moveTo`、`lineTo`、`quadraticCurveTo`、`bezierCurveTo`、`rect`、`closePath` 等标准绘图指令。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `d3-path`，无需改名：

```typescript
// TypeScript / Node.js (npm: d3-path)
import { path } from "d3-path";
const p = path();
p.moveTo(0, 0);
p.lineTo(100, 50);
p.closePath();
const d = p.toString(); // "M0,0L100,50Z"
```

```cp
// CP（零堆分配，内置 2048 字节栈缓冲区）
import { SvgPath } from "d3-path";

let p = SvgPath.new();
p.moveTo(0.0, 0.0);
p.lineTo(100.0, 50.0);
p.closePath();

let out: [128]u8 = undefined;
let n = p.writeTo(out[0..128]);
let d = out[0..n] as string; // "M0,0L100,50Z"
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "d3-path": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `SvgPath.new` | `(): SvgPath` | 创建空路径构建器（内置 2048 字节栈缓冲区） |
| `SvgPath.moveTo` | `(x: f64, y: f64)` | 输出 `M x,y` 并开启新子路径 |
| `SvgPath.lineTo` | `(x: f64, y: f64)` | 输出 `L x,y`（尚无子路径时等价于 `M x,y`） |
| `SvgPath.quadraticCurveTo` | `(cpx: f64, cpy: f64, x: f64, y: f64)` | 输出二次贝塞尔曲线 `Q cpx,cpy,x,y` |
| `SvgPath.bezierCurveTo` | `(cp1x: f64, cp1y: f64, cp2x: f64, cp2y: f64, x: f64, y: f64)` | 输出三次贝塞尔曲线 `C cp1x,cp1y,cp2x,cp2y,x,y` |
| `SvgPath.rect` | `(x: f64, y: f64, w: f64, h: f64)` | 输出轴对齐矩形 `M x,y h w v h h -w Z` |
| `SvgPath.closePath` | `()` | 输出 `Z` 并将当前点重置回子路径起点 |
| `SvgPath.byteLen` | `(): usize` | 获取当前已生成的路径字节长度 |
| `SvgPath.writeTo` | `(out: []u8): usize` | 将路径字符串拷贝到调用方 `out` 缓冲区 |
| `SvgPath.reset` | `()` | 清空路径状态，零成本复用同一实例 |
| `d3pPushStr` / `d3pPushU64` / `d3pPushNum` | 底层写入辅助函数 | 紧凑数字格式化（最多保留 3 位小数并自动去除尾零） |

## 实战示例

### 1. 曲线绘制、矩形与实例复用

```cp
import { SvgPath } from "d3-path";

let p = SvgPath.new();
p.moveTo(10.0, 20.0);
p.quadraticCurveTo(50.0, 0.0, 90.0, 20.0);
p.bezierCurveTo(100.0, 40.0, 110.0, 60.0, 120.0, 80.0);
p.closePath();

let out: [256]u8 = undefined;
let n1 = p.writeTo(out[0..256]);
let curve_d = out[0..n1] as string;

// 重置并复用同一个 SvgPath 实例
p.reset();
p.rect(0.0, 0.0, 64.5, 32.25);
let n2 = p.writeTo(out[0..256]);
let rect_d = out[0..n2] as string;
```

## 注意事项与边界说明

- **固定栈容量**：`SvgPath.buf` 固定为 `[2048]u8`，绘制大量独立图形时可通过 `p.reset()` 复用。
- **紧凑浮点格式**：坐标最多保留 3 位小数并自动去掉多余的 `0`（如 `10.0` 输出 `"10"`，`32.250` 输出 `"32.25"`）。
- **零堆分配**：适用于高性能图表渲染、服务端 SVG 生成与实时矢量图形管线。
