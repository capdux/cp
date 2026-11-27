[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shlex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard (shell-quote / Rust shlex parity) for CP

## 安装与导入

```bash
cap add shlex
```

```cp
import { needsQuote } from "shlex";
```

## 核心导出 API

- `export enum ShlexError`
- `export fn needsQuote(arg: string): bool;`
- `export fn hasShellMetachars(s: string): bool;`
- `export fn quoteArg(arg: string, out: []u8): string;`
- `export fn joinArgs(args: []string, out: []u8): string;`
- `export fn splitShell(cmd: string): SplitResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shell-quote@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`shell-quote@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`shlex@1.0.0`)，许可证：MIT
