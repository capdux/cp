[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zeroize

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap secure secret memory wiping and ZeroizingBuf32/ZeroizingBuf64 key containers (crates.io zeroize equivalent)

## 安装与导入

```bash
cap add zeroize
```

```cp
import { zeroizeBytes } from "zeroize";
```

## 核心导出 API

- `export fn zeroizeBytes(buf: []u8): usize;`
- `export fn zeroizeU32(buf: []u32): usize;`
- `export fn zeroizeU64(buf: []u64): usize;`
- `export fn isAllZeroBytes(buf: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zeroize@1.0.0`)，许可证：Apache-2.0 OR MIT
