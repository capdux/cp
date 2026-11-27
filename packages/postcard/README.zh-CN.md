[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# postcard

> 版本：`1.1.3` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap compact binary wire protocol codecs (Varint, ZigZag, and COBS framing)

## 安装与导入

```bash
cap add postcard
```

```cp
import { pcrdU64Max } from "postcard";
```

## 核心导出 API

- `export fn pcrdU64Max(): u64;`
- `export fn encodeVarintU64(val: u64, out: []u8): usize;`
- `export fn decodeVarintU64(data: &[]u8): VarintU64Res;`
- `export fn zigzagEncode(val: i64): u64;`
- `export fn zigzagDecode(val: u64): i64;`
- `export fn encodeVarintI64(val: i64, out: []u8): usize;`
- `export fn decodeVarintI64(data: &[]u8): VarintI64Res;`
- `export fn cobsEncode(src: &[]u8, out: []u8): usize;`
- `export fn cobsDecode(src: &[]u8, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`postcard@1.1.3`)（https://github.com/jamesmunns/postcard），许可证：MIT
