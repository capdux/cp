[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# smol_str

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP O(1)-clone small string optimization (24-byte inline SSO + read-only slice fallback, crates.io smol_str equivalent)

## 安装与导入

```bash
cap add smol_str
```

```cp
import { smolGetByte } from "smol_str";
```

## 核心导出 API

- `export fn smolGetByte(s: SmolStr, idx: usize): u8;`
- `export fn smolMinUsize(a: usize, b: usize): usize;`
- `export fn smolStartsWith(s: SmolStr, prefix: string): bool;`
- `export fn smolEndsWith(s: SmolStr, suffix: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`smol_str@1.0.0`)，许可证：MIT OR Apache-2.0
