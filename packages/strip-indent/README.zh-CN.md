[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strip-indent

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation leading whitespace stripper (npm strip-indent + min-indent parity) for CP

## 安装与导入

```bash
cap add strip-indent
```

```cp
import { siIsIndentChar } from "strip-indent";
```

## 核心导出 API

- `export fn siIsIndentChar(ch: u8): bool;`
- `export fn siCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;`
- `export fn minIndent(s: string): usize;`
- `export fn stripIndentBy(s: string, n: usize, out: []u8): usize;`
- `export fn stripIndent(s: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strip-indent@1.0.0`)，许可证：MIT
