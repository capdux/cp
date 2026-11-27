[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# slice-ansi

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI-preserving terminal column slicer and truncator with CJK double-width support in pure CP

## 安装与导入

```bash
cap add slice-ansi
```

```cp
import { ansiVisibleWidth } from "slice-ansi";
```

## 核心导出 API

- `export fn ansiVisibleWidth(input: string): usize;`
- `export fn sliceAnsi(input: string, begin: usize, end: usize, out: []u8): usize;`
- `export fn sliceAnsiFrom(input: string, begin: usize, out: []u8): usize;`
- `export fn truncateAnsi(input: string, max_cols: usize, ellipsis: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`slice-ansi@1.0.0`)，许可证：MIT
