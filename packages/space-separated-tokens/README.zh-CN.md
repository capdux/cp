[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# space-separated-tokens

> 版本：`2.0.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTML/SVG space-separated and comma-separated token list parser and serializer

## 安装与导入

```bash
cap add space-separated-tokens
```

```cp
import { parseSpaceTokens } from "space-separated-tokens";
```

## 核心导出 API

- `export fn parseSpaceTokens(input: string, out: []string): usize;`
- `export fn hasSpaceToken(input: string, target: string): bool;`
- `export fn stringifySpaceTokens(tokens: &[]string, out: []u8): usize;`
- `export fn parseCommaTokens(input: string, out: []string): usize;`
- `export fn stringifyCommaTokens(tokens: &[]string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`space-separated-tokens@2.0.2`)（https://github.com/wooorm/space-separated-tokens），许可证：MIT
