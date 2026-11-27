[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# acorn

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ES2024 JavaScript lexer and ES Module import/export scanner (npm acorn v8 parity) for CP

## 安装与导入

```bash
cap add acorn
```

```cp
import { analyzeJsModule } from "acorn";
```

## 核心导出 API

- `export fn analyzeJsModule(src: string): JsModuleSummary;`
- `export fn extractImportSpecifiers(src: string, out_specs: []string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`acorn@1.0.0`)，许可证：MIT
