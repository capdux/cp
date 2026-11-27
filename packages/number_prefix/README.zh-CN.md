[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# number_prefix

> 版本：`0.4.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap SI decimal (k/M/G/T/P/E) and IEC binary (Ki/Mi/Gi/Ti/Pi/Ei) numeric prefix formatter

## 安装与导入

```bash
cap add number_prefix
```

```cp
import { decimalPrefix } from "number_prefix";
```

## 核心导出 API

- `export enum PrefixKind`
- `export fn decimalPrefix(amount: f64): PrefixedNumber;`
- `export fn binaryPrefix(amount: f64): PrefixedNumber;`
- `export fn formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`number_prefix@0.4.0`)（https://github.com/ogham/rust-number-prefix），许可证：MIT
