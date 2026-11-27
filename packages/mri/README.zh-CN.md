[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mri

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX/GNU CLI argument parser (--flag, --key=val, --key val, -abc short clusters, -- terminator, stdin '-') in pure CP

## 安装与导入

```bash
cap add mri
```

```cp
import { strEq } from "mri";
```

## 核心导出 API

- `export fn strEq(a: string, b: string): bool;`
- `export fn parseArgs(argv: &[]string, out: []ParsedArg): usize;`
- `export fn hasFlag(argv: &[]string, long_name: string, short_char: u8): bool;`
- `export fn getOption(argv: &[]string, long_name: string, short_char: u8): string;`
- `export fn countPositionals(argv: &[]string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mri@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`lexopt@1.0.0`)，许可证：MIT
