[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shell-quote

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap POSIX shell argument quoting and command-line tokenization in pure CP (modeled after npm shell-quote)

## 安装与导入

```bash
cap add shell-quote
```

```cp
import { isSafeBareChar } from "shell-quote";
```

## 核心导出 API

- `export fn isSafeBareChar(c: u8): bool;`
- `export fn needsQuoting(arg: &[]u8): bool;`
- `export fn quoteArg(arg: &[]u8, dst: []u8): i64;`
- `export fn quoteArgs(args: &[]&[]u8, dst: []u8): i64;`
- `export fn splitCommand(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shell-quote@1.0.0`)，许可证：MIT
