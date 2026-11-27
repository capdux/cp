[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/libvpx

> 版本：`1.15.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Google libvpx 1.15.0 royalty-free VP8/VP9 video codec for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/libvpx
```

```cp
import { cp_vpx_version_str } from "@cap/libvpx";
```

## 核心导出 API

- `export fn cp_vpx_version_str(): [*:0]&u8!;`
- `export fn cp_vpx_vp8_enc_cfg_default(out_w: *u32!, out_h: *u32!): i32;`
- `export fn cp_vpx_vp9_dec_open_close(threads: i32): i32;`
- `export fn versionStr(): string;`
- `export fn vp8EncCfgDefault(): !Vp8DefaultSize;`
- `export fn vp9DecOpenClose(threads: i32): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`libvpx@1.15.0`)，许可证：BSD-3-Clause
