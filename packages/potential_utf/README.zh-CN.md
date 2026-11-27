[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# potential_utf

> 版本：`1.0.0` | 许可证：`Unicode-3.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers (ICU4X potential_utf parity) for CP

## 安装与导入

```bash
cap add potential_utf
```

```cp
import { putfIsCont } from "potential_utf";
```

## 核心导出 API

- `export fn putfIsCont(b: u8): bool;`
- `export fn putfValidSeqLen(bytes: &[]u8, pos: usize): usize;`
- `export fn putfMaximalSubpart(bytes: &[]u8, pos: usize): usize;`
- `export fn putfEncodeScalar(scalar: u32, out: []u8, w: usize): usize;`
- `export fn putfWriteReplacement(out: []u8, w: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`potential_utf@1.0.0`)，许可证：Unicode-3.0
