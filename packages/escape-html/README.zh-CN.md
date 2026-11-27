[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# escape-html

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 escape-html

## 安装与导入

```bash
cap add escape-html
```

```cp
import { needsHtmlEscape } from "escape-html";
```

## 核心导出 API

- `export fn needsHtmlEscape(input: string): bool;`
- `export fn escapedHtmlLen(input: string): usize;`
- `export fn escapeHtml(input: string, out: []u8): usize;`
- `export fn unescapeHtml(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`escape-html@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`entities@1.0.0`)，许可证：MIT
