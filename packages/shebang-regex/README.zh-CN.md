[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# shebang-regex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation shebang (#!) line matcher and stripper for CP

## 安装与导入

```bash
cap add shebang-regex
```

```cp
import { hasShebang } from "shebang-regex";
```

## 核心导出 API

- `export fn hasShebang(input: string): bool;`
- `export fn matchShebang(input: string): ShebangMatch;`
- `export fn stripShebang(input: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`shebang-regex@1.0.0`)，许可证：MIT
