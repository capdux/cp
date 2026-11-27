[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bendy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

BitTorrent BEP-3 deterministic binary Bencode streaming encoder, zero-copy decoder, validator, and dict lookup.

## 安装与导入

```bash
cap add bendy
```

```cp
import { peekKind } from "bendy";
```

## 核心导出 API

- `export fn peekKind(src: &[]u8, pos: usize): u8;`
- `export fn decodeInt(src: &[]u8, pos: usize): BencodeIntResult;`
- `export fn decodeBytes(src: &[]u8, pos: usize): BencodeBytesResult;`
- `export fn compareBytes(a: &[]u8, b: &[]u8): i32;`
- `export fn parseValueEnd(src: &[]u8, pos: usize, depth: usize): i64;`
- `export fn validate(src: &[]u8): bool;`
- `export fn dictLookup(src: &[]u8, key: &[]u8): BencodeBytesResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bendy@1.0.0`)，许可证：MIT
