[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# miniz_oxide

> 版本：`3.0.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Single-file lossless compression & ZIP archive creation/extraction engine (miniz 3.0.2) for CP

## 安装与导入

```bash
cap add miniz_oxide
```

```cp
import { cp_miniz_version } from "miniz_oxide";
```

## 核心导出 API

- `export fn cp_miniz_version(): [*:0]&u8!;`
- `export fn cp_miniz_zip_create_single(entry_name: [*:0]&u8!, data: [*]&u8!, data_len: usize, out_zip_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_zip_extract_entry(zip_bytes: [*]&u8!, zip_len: usize, entry_name: [*:0]&u8!, out_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_free(ptr: *auto!?);`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`miniz_oxide@3.0.2`)，许可证：MIT
