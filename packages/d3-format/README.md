# @cap/d3-format

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-format` ([https://github.com/d3/d3-format](https://github.com/d3/d3-format))（原库名 `d3-format` → CP 包名 `@cap/d3-format`）

零堆分配的 CP 原生数值格式化规范解析与渲染引擎，Clean-Room ISC 实现对标 D3.js 顶流格式化库 `d3-format`（1.10 亿+/周下载）。

完整支持 Python/D3 格式语法 `[[fill]align][sign][symbol][0][width][,][.precision][~][type]`，覆盖定点浮点（`f`）、千分位分组（`,`）、百分比（`%`）、SI 国际单位缩放（`s`：`y`..`µ`/`m`/`k`/`M`/`G`..`Y`）、整数（`d`）、十六进制/二进制/八进制（`x`/`X`/`b`/`o` 及 `#` 前缀）、科学计数法（`e`/`E`）以及宽度对齐与零填充。

## 核心 API

- `FormatSpec` — 格式规范解析结果结构体（`valid`, `fill`, `align`, `sign`, `symbol`, `zero`, `width`, `comma`, `precision`, `trim`, `ty`）
- `parseFormatSpec(spec: string): FormatSpec` — 解析格式串（如 `",.2f"`、`"+08d"`、`".1%"`、`".2s"`、`"#x"`、`">10.2f"`）
- `formatNumber(spec: string, value: f64, out: []u8): usize` — 按格式规范将 `f64` 零堆渲染至目标缓冲并返回写入字节数

## 致谢

Clean-Room ISC 实现，API 语义对标开源生态 [`d3/d3-format`](https://github.com/d3/d3-format)。
