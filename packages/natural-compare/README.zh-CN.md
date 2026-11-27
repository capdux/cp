[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# natural-compare

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation natural / alphanumeric string comparison and in-place sorting (natural-compare / natsort / natord parity, arbitrary-length integer segments without overflow) for CP

## 安装与导入

```bash
cap add natural-compare
```

```cp
import { compareRawBytes } from "natural-compare";
```

## 核心导出 API

- `export fn compareRawBytes(a: string, b: string): i8;`
- `export fn naturalCompareImpl(a: string, b: string, ignore_case: bool): i8;`
- `export fn naturalCompare(a: string, b: string): i8;`
- `export fn naturalCompareIgnoreCase(a: string, b: string): i8;`
- `export fn naturalLess(a: string, b: string): bool;`
- `export fn naturalLessIgnoreCase(a: string, b: string): bool;`
- `export fn naturalEq(a: string, b: string): bool;`
- `export fn comesBefore(a: string, b: string, desc: bool, ignore_case: bool): bool;`
- `export fn siftDown(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool);`
- `export fn sortInPlace(items: []string, desc: bool, ignore_case: bool);`
- `export fn sortNatural(items: []string);`
- `export fn sortNaturalDesc(items: []string);`
- `export fn sortNaturalIgnoreCase(items: []string);`
- `export fn isSortedNatural(items: []string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`natural-compare@1.0.0`)，许可证：MIT
