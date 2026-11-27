[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ini

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap INI / .gitconfig / .npmrc / .editorconfig parser and writer with sections, comments, quoted values, and typed getters.

## 安装与导入

```bash
cap add ini
```

```cp
import { isWs } from "ini";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ini@1.0.0`)，许可证：ISC
