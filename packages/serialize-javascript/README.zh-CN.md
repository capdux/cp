[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# serialize-javascript

> 版本：`6.0.2` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap XSS-safe JSON/JavaScript literal serializer escaping HTML script breakouts and line separators

## 安装与导入

```bash
cap add serialize-javascript
```

```cp
import { escapeUnsafeJson } from "serialize-javascript";
```

## 核心导出 API

- `export fn escapeUnsafeJson(json_text: string, out: []u8): usize;`
- `export fn serializeString(s: string, out: []u8): usize;`
- `export fn hasUnsafeScriptChars(s: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`serialize-javascript@6.0.2`)（https://github.com/yahoo/serialize-javascript），许可证：BSD-3-Clause
