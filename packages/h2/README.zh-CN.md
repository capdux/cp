[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# h2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7541 HTTP/2 HPACK static table (61 entries), N-bit prefixed integer codec, and indexed header encoder for CP.

## 安装与导入

```bash
cap add h2
```

```cp
import { staticTableGet } from "h2";
```

## 核心导出 API

- `export fn staticTableGet(index: usize): StaticEntry;`
- `export fn staticTableFind(name: string, value: string): usize;`
- `export fn staticTableFindName(name: string): usize;`
- `export fn encodePrefixedInt(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize;`
- `export fn decodePrefixedInt(data: &[]u8, prefix_bits: u8): IntDecodeResult;`
- `export fn encodeIndexedHeader(index: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`h2@1.0.0`)，许可证：MIT
