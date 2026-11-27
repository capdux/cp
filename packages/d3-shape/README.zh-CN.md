[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-shape

> 版本：`3.2.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP SVG shape and pie chart path generator (line, step, arc, pie)

## 安装与导入

```bash
cap add d3-shape
```

```cp
import { d3shWriteByte } from "d3-shape";
```

## 核心导出 API

- `export fn d3shWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn d3shWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn d3shWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn d3shWriteFloat(out: []u8, pos: usize, val: f64): usize;`
- `export fn d3shWritePair(out: []u8, pos: usize, x: f64, y: f64): usize;`
- `export fn computePie(values: &[]f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize;`
- `export fn linePath(pts: &[]Point2D, out: []u8): usize;`
- `export fn stepLinePath(pts: &[]Point2D, out: []u8): usize;`
- `export fn arcPath(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-shape@3.2.0`)（https://github.com/d3/d3-shape），许可证：ISC
