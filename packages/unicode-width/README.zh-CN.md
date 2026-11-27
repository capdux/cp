[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unicode-width

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode 15.1 East Asian Width & terminal display column width calculator with UTF-8 safe truncation for CP.

## 安装与导入

```bash
cap add unicode-width
```

```cp
import { charWidth } from "unicode-width";
```

## 核心导出 API

- `export fn charWidth(cp: u32): usize;`
- `export fn charWidthCjk(cp: u32): usize;`
- `export fn strWidth(s: string): usize;`
- `export fn strWidthCjk(s: string): usize;`
- `export fn truncateToWidth(s: string, max_cols: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unicode-width@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`eastasianwidth@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`is-fullwidth-code-point@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`wcwidth@1.0.0`)，许可证：MIT
