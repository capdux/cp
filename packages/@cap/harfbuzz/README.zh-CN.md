[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/harfbuzz

> 版本：`15.2.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

HarfBuzz (v15.2.0) industrial-standard OpenType text shaping engine (CJK/Arabic/Latin/Indic) for CP

## 安装与导入

```bash
cap add @cap/harfbuzz
```

```cp
import { cp_hb_version_string } from "@cap/harfbuzz";
```

## 核心导出 API

- `export fn cp_hb_version_string(): [*:0]&u8!;`
- `export fn cp_hb_buffer_create(): *auto!?;`
- `export fn cp_hb_buffer_destroy(buf: *auto!?);`
- `export fn cp_hb_buffer_reset(buf: *auto!?);`
- `export fn cp_hb_buffer_add_utf8(buf: *auto!?, text: [*]&u8!, len: i32);`
- `export fn cp_hb_buffer_guess_segment_properties(buf: *auto!?);`
- `export fn cp_hb_buffer_get_length(buf: *auto!?): u32;`
- `export fn cp_hb_buffer_get_direction(buf: *auto!?): i32;`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`harfbuzz@15.2.0`)，许可证：MIT
