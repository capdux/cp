[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tzfile

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser, and UTC offset transition evaluator (crates.io tzfile equivalent)

## 安装与导入

```bash
cap add tzfile
```

```cp
import { tzReadU32Be } from "tzfile";
```

## 核心导出 API

- `export fn tzReadU32Be(buf: []u8, off: usize): u32;`
- `export fn isTzifMagic(buf: []u8): bool;`
- `export fn parseTzifHeader(buf: []u8): TzifHeader?;`
- `export fn tzIsAlpha(c: u8): bool;`
- `export fn tzIsDigit(c: u8): bool;`
- `export fn tzParseAbbr(s: string, pos_in: usize): TzAbbrRes;`
- `export fn tzParsePosixOffset(s: string, pos_in: usize): TzOffRes;`
- `export fn parsePosixTz(spec: string): PosixTz?;`
- `export fn lookupTransitionOffset(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32;`
- `export fn utcToLocalSeconds(unix_secs: i64, offset_secs: i32): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tzfile@1.0.0`)，许可证：MIT OR Apache-2.0
