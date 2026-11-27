[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zbase32

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Zooko human-oriented z-base-32 encoder, decoder, and validator in pure CP

## 安装与导入

```bash
cap add zbase32
```

```cp
import { toLowerAscii } from "zbase32";
```

## 核心导出 API

- `export fn toLowerAscii(b: u8): u8;`
- `export fn decodeChar(b: u8): i32;`
- `export fn encodedLen(byte_len: usize): usize;`
- `export fn encode(data: &[]u8, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
- `export fn isValidZBase32(input: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`zbase32@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`zbase32@1.0.0`)，许可证：MIT
