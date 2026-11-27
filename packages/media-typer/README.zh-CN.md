[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# media-typer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 6838 HTTP media type parser, validator, and formatter (clean-room port of npm media-typer)

## 安装与导入

```bash
cap add media-typer
```

```cp
import { mtIsAlphaNum } from "media-typer";
```

## 核心导出 API

- `export fn mtIsAlphaNum(b: u8): bool;`
- `export fn mtIsRestrictedChar(b: u8): bool;`
- `export fn mtIsValidName(s: string): bool;`
- `export fn mtPushLower(out: []u8, pos: usize, s: string): usize;`
- `export fn testMediaType(raw: string): bool;`
- `export fn parseMediaType(raw: string): MediaType;`
- `export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;`
- `export fn mediaTypeEq(a: string, b: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`media-typer@1.0.0`)，许可证：MIT
