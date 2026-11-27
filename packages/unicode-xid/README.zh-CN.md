[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-xid

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classification and identifier scanner for CP (clean-room MIT).

## 安装与导入

```bash
cap add unicode-xid
```

```cp
import { uxidDecodeUtf8Strict } from "unicode-xid";
```

## 核心导出 API

- `export fn uxidDecodeUtf8Strict(s: string, offset: usize): UxidRune;`
- `export fn uxidIsExtraStart(cp: u32): bool;`
- `export fn uxidBsearchStart(cp: u32): bool;`
- `export fn uxidBsearchCont(cp: u32): bool;`
- `export fn uxidIsAsciiAlpha(b: u8): bool;`
- `export fn uxidIsAsciiDigit(b: u8): bool;`
- `export fn isXidStart(cp: u32): bool;`
- `export fn isXidContinue(cp: u32): bool;`
- `export fn isIdentStart(cp: u32): bool;`
- `export fn isAsciiIdent(s: string): bool;`
- `export fn isValidIdent(s: string): bool;`
- `export fn scanIdentLen(s: string, start: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-xid@1.0.0`)，许可证：MIT
