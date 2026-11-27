[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# change-case

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap full-featured case conversion library covering all 12 change-case v5 styles (camel, pascal, snake, kebab, constant, dot, path, train, capital, sentence, no, swap).

## 安装与导入

```bash
cap add change-case
```

```cp
import { camelCase } from "change-case";
```

## 核心导出 API

- `export fn camelCase(input: string, out: []u8): usize;`
- `export fn pascalCase(input: string, out: []u8): usize;`
- `export fn snakeCase(input: string, out: []u8): usize;`
- `export fn kebabCase(input: string, out: []u8): usize;`
- `export fn constantCase(input: string, out: []u8): usize;`
- `export fn dotCase(input: string, out: []u8): usize;`
- `export fn pathCase(input: string, out: []u8): usize;`
- `export fn trainCase(input: string, out: []u8): usize;`
- `export fn capitalCase(input: string, out: []u8): usize;`
- `export fn sentenceCase(input: string, out: []u8): usize;`
- `export fn noCase(input: string, out: []u8): usize;`
- `export fn swapCase(input: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`change-case@1.0.0`)，许可证：MIT
