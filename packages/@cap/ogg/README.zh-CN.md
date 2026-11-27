[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/ogg

> 版本：`1.3.5` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Xiph libogg 1.3.5 bitstream container framing and page muxing engine for CP

## 安装与导入

```bash
cap add @cap/ogg
```

```cp
import { cp_ogg_stream_init } from "@cap/ogg";
```

## 核心导出 API

- `export fn cp_ogg_stream_init(serialno: i32): *auto!?;`
- `export fn cp_ogg_stream_packetin(os: *auto!?, data: [*]&u8!, len: isize, bos: i32, eos: i32, granulepos: i64, packetno: i64): i32;`
- `export fn cp_ogg_stream_pageout(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;`
- `export fn cp_ogg_stream_flush(os: *auto!?, out_buf: [*]u8!, out_cap: usize, out_len: *usize!): i32;`
- `export fn cp_ogg_stream_clear(os: *auto!?);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`ogg@1.3.5`)，许可证：BSD-3-Clause
