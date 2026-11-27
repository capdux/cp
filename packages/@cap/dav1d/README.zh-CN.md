[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/dav1d

> 版本：`1.5.4` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

VideoLAN dav1d (v1.5.4) high-performance royalty-free AV1 video decoder for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/dav1d
```

```cp
import { cp_dav1d_version } from "@cap/dav1d";
```

## 核心导出 API

- `export fn cp_dav1d_version(): [*:0]&u8!;`
- `export fn cp_dav1d_version_api(): u32;`
- `export fn cp_dav1d_open(n_threads: i32, max_frame_delay: i32): *auto!?;`
- `export fn cp_dav1d_flush(ctx: *auto!?);`
- `export fn cp_dav1d_close(ctx: *auto!?);`
- `export fn version(): string;`
- `export fn versionApi(): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`dav1d@1.5.4`)，许可证：BSD-2-Clause
