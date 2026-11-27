[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# quinn-proto

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 9000 QUIC variable-length integer codec and Long Header parser (quinn-proto parity)

## 安装与导入

```bash
cap add quinn-proto
```

```cp
import { encodedLen } from "quinn-proto";
```

## 核心导出 API

- `export fn encodedLen(val: u64): usize;`
- `export fn encodeVarint(val: u64, out: []u8): usize;`
- `export fn decodeVarint(buf: []u8): QuicVarint;`
- `export fn parseLongHeader(pkt: []u8): QuicLongHeader;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`quinn-proto@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`quinn@1.0.0`)，许可证：MIT
