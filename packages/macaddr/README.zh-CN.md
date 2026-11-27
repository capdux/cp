[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# macaddr

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 (MacAddr6) and EUI-64 (MacAddr8) hardware address parser, formatter, and RFC 4291 converter (crates.io macaddr equivalent)

## 安装与导入

```bash
cap add macaddr
```

```cp
import { macHexVal } from "macaddr";
```

## 核心导出 API

- `export fn macHexVal(ch: u8): u8?;`
- `export fn macParseHexByte(hi_ch: u8, lo_ch: u8): u8?;`
- `export fn macHexDigit(n: u8): u8;`
- `export fn macWriteHexByte(b: u8, out: []u8, pos: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`macaddr@1.0.0`)，许可证：MIT OR Apache-2.0
