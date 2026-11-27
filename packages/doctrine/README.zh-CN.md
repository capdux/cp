[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# doctrine

> 版本：`1.0.0` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation JSDoc comment parser extracting description and @param/@returns/@type/@deprecated/@throws/@template tags (npm doctrine parity) for CP

## 安装与导入

```bash
cap add doctrine
```

```cp
import { docIsSpace } from "doctrine";
```

## 核心导出 API

- `export fn docIsSpace(ch: u8): bool;`
- `export fn docStripLinePrefix(line: string): string;`
- `export fn docTagHasName(title: string): bool;`
- `export fn docParseTagLine(line: string): JsDocTag;`
- `export fn parseJsDoc(comment: string): JsDocComment;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`doctrine@1.0.0`)，许可证：Apache-2.0
