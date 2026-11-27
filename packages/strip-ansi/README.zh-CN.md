[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strip-ansi

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap ECMA-48 / ANSI terminal escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter.

## 安装与导入

```bash
cap add strip-ansi
```

```cp
import { strippedLen } from "strip-ansi";
```

## 核心导出 API

- `export fn strippedLen(input: string): usize;`
- `export fn stripAnsi(input: string, out: []u8): usize;`
- `export fn visibleAsciiCount(input: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strip-ansi@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`strip-ansi-escapes@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`string-length@1.0.0`)，许可证：MIT
