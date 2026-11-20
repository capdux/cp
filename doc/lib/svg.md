# CP 标准库：矢量 SVG 1.1 画布与统计图表引擎手册 (`cap:svg`)

## 一、 架构总览与设计准则

`cap:svg` 是 CP 语言官方内置的纯 CP 矢量 SVG 1.1 绘图画布与声明式统计图表引擎，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP AI Agent 数据分析、可观测性仪表盘与报告导出场景设计。既可通过 `import { ... } from "cap:svg"` 导入，也支持 `import cap from "cap"; cap.svg.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP 纯正矢量 SVG 1.1 引擎 (cap:svg)                    │
├──────────────────────────────┬──────────────────────────────────────────┤
│ 矢量画布与路径构建 (canvas.cp)│ 声明式统计图表引擎 (charts.cp)            │
│ SvgCanvas / SvgPathBuilder   │ SvgCharts.barChart (正负/零值 + 刻度)     │
│ SvgStyle / SvgPoint          │ SvgCharts.lineChart (多系列归一化 + 图例) │
│ 分组嵌套 / XSS 严格 XML 转义   │ SvgCharts.pieChart (饼图/环形图/100% 边界) │
└──────────────────────────────┴──────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/svg/canvas.cp` | `// Source: cp/lib/svg/canvas.cp (SVG 1.1 Canvas, Path Builder & XSS-Safe Serializer)` | `SvgError`、`SvgPoint`、`SvgStyle`、`SvgPathBuilder`、`SvgCanvas` |
| `lib/svg/charts.cp` | `// Source: cp/lib/svg/charts.cp (Declarative Statistical Charts Engine: Bar, Line, Pie/Donut)` | `ChartDatum`、`LineSeries`、`SvgCharts` |
| `lib/svg/index.cp` | `// Source: cp/lib/svg/index.cp (Unified Exports for cap:svg)` | `cap:svg` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与基础类型（`canvas.cp`）

- **`SvgError`**：`InvalidDimensions`（宽/高非有限或 `<= 0`）、`EmptySeries`（空数据集或空点集）、`InvalidValue`（`NaN`/`Inf` 或非法负值/越界环形比）、`OutOfMemory`。
- **`SvgPoint`**：`x: f64 = 0.0`、`y: f64 = 0.0`。
- **`SvgStyle`**：
  - `fill: string = "none"`
  - `stroke: string = "none"`
  - `stroke_width: f64 = 0.0`
  - `opacity: f64 = 1.0`
  - `font_size: f64 = 12.0`
  - `anchor: string = ""`（`start` / `middle` / `end`）

### 2. 流式路径构建器（`SvgPathBuilder`）

- `SvgPathBuilder.init(alloc)` / `deinit()`
- `moveTo(x, y): ! SvgError` -> `M x y`
- `lineTo(x, y): ! SvgError` -> `L x y`
- `curveTo(cx1, cy1, cx2, cy2, x, y): ! SvgError` -> `C cx1 cy1 cx2 cy2 x y`
- `arcTo(rx, ry, x_axis_rotation, large_arc, sweep, x, y): ! SvgError` -> `A rx ry rot large sweep x y`
- `close(): ! SvgError` -> `Z`
- `slice(): string` / `toOwnedSlice(): ([]u8) ! SvgError`

### 3. 矢量画布（`SvgCanvas`）

- `SvgCanvas.init(alloc, width, height): SvgCanvas ! SvgError` / `deinit()`：校验 `width > 0 && height > 0` 且为有限浮点数；
- `setBackground(color)`：设置整画布背景 `<rect width="100%" height="100%" .../>`；
- `beginGroup(id, transform)` / `endGroup()`：打开与关闭 `<g>` 分组；`render()` 时若仍有未闭合分组将自动补齐 `</g>`；
- 图元方法（均自动校验有限数值并执行严格 XML `&`/`<`/`>`/`"`/`'` 转义，防止 SVG/XSS 注入）：
  - `rect(x, y, w, h, rx, style)`
  - `circle(cx, cy, r, style)`
  - `line(x1, y1, x2, y2, style)`
  - `polyline(points, style)`
  - `path(d, style)`
  - `text(x, y, content, style)`
- `render(alloc): ([]u8) ! SvgError`：生成标准 `<svg xmlns="http://www.w3.org/2000/svg" width="..." height="..." viewBox="0 0 W H">...</svg>` 文本（可被 `cap:xml` 直接解析为合法 DOM）。

### 4. 声明式统计图表引擎（`charts.cp`）

- **`ChartDatum`**：`label: string`、`value: f64`、`color: string`（空字符串时自动使用内置 8 色调色板）；
- **`LineSeries`**：`name: string`、`points: []const SvgPoint`、`color: string`；
- **`SvgCharts.barChart(alloc, title, data, width, height): ([]u8) ! SvgError`**：
  - 自动使用 Heckbert Nice Ticks 算法划分 Y 轴网格刻度；
  - 原生支持正值、负值（自动绘制 `y = 0` 基线，负柱向下延伸）及全零值；
- **`SvgCharts.lineChart(alloc, title, series, width, height): ([]u8) ! SvgError`**：
  - 自动统一计算全部折线系列的全局 X/Y 极值域与 Nice Ticks 网格；
  - 绘制 `<polyline>` 折线、数据圆点 `<circle>` 与顶部图例 `legend`；
- **`SvgCharts.pieChart(alloc, title, data, width, height, donut_ratio): ([]u8) ! SvgError`**：
  - `donut_ratio == 0.0` 生成实心饼图；`0.0 < donut_ratio < 1.0` 生成环形 Donut 图；
  - 内置 100% 单全圆弧段边界拆分（双 `180°` 半圆弧 `<path A ...>`），避免起终点重合导致 SVG 弧线退化消失。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SvgCanvas, SvgPathBuilder, SvgStyle, SvgCharts, ChartDatum } from "cap:svg";
import cap from "cap";

pub fn demoSvg(): ! {
    const alloc = std.heap.page_allocator;

    // 1. 画布与路径绘制
    let canvas = SvgCanvas.init(alloc, 400.0, 200.0)?;
    defer canvas.deinit();
    canvas.setBackground("#f8fafc");

    let pb = SvgPathBuilder.init(alloc);
    defer pb.deinit();
    pb.moveTo(20.0, 150.0)?;
    pb.lineTo(120.0, 40.0)?;
    pb.lineTo(220.0, 150.0)?;
    pb.close()?;
    canvas.path(pb.slice(), SvgStyle{ fill: "#3b82f6", stroke: "#1e40af", stroke_width: 2.0 })?;

    const raw_svg = canvas.render(alloc)?;
    defer alloc.free(raw_svg);

    // 2. 动态挂载 cap.svg 绘制柱状图
    const bars: [3]ChartDatum = [
        ChartDatum{ label: "Q1", value: 120.0, color: "" },
        ChartDatum{ label: "Q2", value: -35.0, color: "#ef4444" },
        ChartDatum{ label: "Q3", value: 240.0, color: "" },
    ];
    const chart_svg = cap.svg.SvgCharts.barChart(alloc, "Revenue", bars[0..], 640.0, 360.0)?;
    defer alloc.free(chart_svg);
}
```
