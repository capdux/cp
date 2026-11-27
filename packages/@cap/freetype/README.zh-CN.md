[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/freetype

> 版本：`2.13.3` | 许可证：`FTL` | 类型声明：[src/index.d.cp](./src/index.d.cp)

FreeType 2.13.3 industrial-standard TrueType/SFNT font rasterization engine for CP

## 安装与导入

```bash
cap add @cap/freetype
```

```cp
import { cp_ft_init } from "@cap/freetype";
```

## 核心导出 API

- `export fn cp_ft_init(): *auto!?;`
- `export fn cp_ft_done(lib: *auto!?);`
- `export fn cp_ft_version(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!);`
- `export fn cp_ft_new_memory_face(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?;`
- `export fn cp_ft_done_face(face: *auto!?);`
- `export fn cp_ft_face_num_glyphs(face: *auto!?): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`freetype@2.13.3`)，许可证：FTL
