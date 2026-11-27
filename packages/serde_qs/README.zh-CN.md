[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# serde_qs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation nested bracket query string parser & serializer (crates.io serde_qs v0.13 parity) composing @cap/percent-encoding for CP.

## 安装与导入

```bash
cap add serde_qs
```

```cp
import { parseKeyPath } from "serde_qs";
```

## 核心导出 API

- `export fn parseKeyPath(key: string): QsPath;`
- `export fn parseQs(query: string, out_pairs: []QsPair): usize;`
- `export fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize?;`
- `export fn serializeQs(pairs: &[]QsPair, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`serde_qs@1.0.0`)，许可证：MIT
