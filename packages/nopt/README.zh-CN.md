[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# nopt

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CLI option parser with shorthands, --no-flag negation, and -- terminator (npm nopt parity) in pure CP

## 安装与导入

```bash
cap add nopt
```

```cp
import { nptStrEq } from "nopt";
```

## 核心导出 API

- `export fn nptStrEq(a: string, b: string): bool;`
- `export fn nptHasPrefix(s: string, prefix: string): bool;`
- `export fn nptIndexOfByte(s: string, ch: u8): usize?;`
- `export fn nptStripLeadingDashes(s: string): string;`
- `export fn nptFindShorthand(shorthands: &[]NoptShorthand, short_name: string): string?;`
- `export fn nptIsOptionToken(arg: string): bool;`
- `export fn parseNopt(args: &[]string, shorthands: &[]NoptShorthand): NoptResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`nopt@1.0.0`)，许可证：ISC
