[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-ident

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 unicode-ident

## 安装与导入

```bash
cap add unicode-ident
```

```cp
import { uidInRange } from "unicode-ident";
```

## 核心导出 API

- `export fn uidInRange(cp: u32, lo: u32, hi: u32): bool;`
- `export fn decodeUtf8At(s: string, offset: usize): Utf8Codepoint;`
- `export fn uidIsXidStartNonAscii(cp: u32): bool;`
- `export fn uidIsXidContinueExtra(cp: u32): bool;`
- `export fn isXidStart(cp: u32): bool;`
- `export fn isXidContinue(cp: u32): bool;`
- `export fn isIdentStart(cp: u32): bool;`
- `export fn scanIdentLen(s: string): usize;`
- `export fn isValidIdent(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-ident@1.0.0`)，许可证：MIT OR Apache-2.0
