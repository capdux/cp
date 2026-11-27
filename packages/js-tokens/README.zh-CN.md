[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# js-tokens

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation tiny JavaScript/TypeScript lexer tokenizer (clean-room MIT implementation of npm js-tokens, #61 / 381M wk)

## 安装与导入

```bash
cap add js-tokens
```

```cp
import { nextJsToken } from "js-tokens";
```

## 核心导出 API

- `export enum JsTokenKind`
- `export fn nextJsToken(src: string, pos: usize): JsToken?;`
- `export fn tokenizeJs(src: string, out: []JsToken): usize;`
- `export fn countJsTokens(src: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`js-tokens@1.0.0`)，许可证：MIT
