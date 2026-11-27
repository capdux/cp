[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/whisper

> 版本：`1.9.6` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

whisper.cpp (v1.9.6) local speech-to-text (ASR) engine for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/whisper
```

```cp
import { cp_whisper_version } from "@cap/whisper";
```

## 核心导出 API

- `export fn cp_whisper_version(): [*:0]&u8!;`
- `export fn cp_whisper_print_system_info(): [*:0]&u8!;`
- `export fn cp_whisper_lang_max_id(): i32;`
- `export fn cp_whisper_lang_id(lang: [*:0]&u8!): i32;`
- `export fn cp_whisper_lang_str(id: i32): [*:0]&u8!;`
- `export fn cp_whisper_init_from_file(path: [*:0]&u8!): *auto!?;`
- `export fn cp_whisper_free(ctx: *auto!?);`
- `export fn cp_whisper_n_vocab(ctx: *auto!?): i32;`
- `export fn versionString(): string;`
- `export fn systemInfo(): string;`
- `export fn langMaxId(): i32;`
- `export fn langId(code: string): i32;`
- `export fn langStr(id: i32): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`whisper.cpp@1.9.6`)，许可证：MIT
