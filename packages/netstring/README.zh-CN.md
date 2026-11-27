[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# netstring

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap D. J. Bernstein Netstring (len:payload,) framing encoder and streaming decoder in pure CP

## 安装与导入

```bash
cap add netstring
```

```cp
import { encodedLen } from "netstring";
```

## 核心导出 API

- `export fn encodedLen(payload_len: usize): usize;`
- `export fn encode(payload: &[]u8, dst: []u8): i64;`
- `export fn decode(buf: &[]u8): NetstringFrame;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`netstring@1.0.0`)，许可证：MIT
