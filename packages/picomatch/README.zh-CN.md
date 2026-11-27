[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# picomatch

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation glob matcher & static base scanner (npm picomatch v4 parity) with globstar, char classes, brace expansion, dotfile protection, and negation for CP

## 安装与导入

```bash
cap add picomatch
```

```cp
import { isMatchWithOptions } from "picomatch";
```

## 核心导出 API

- `export fn isMatchWithOptions(path: string, pattern: string, opts: PicoOptions): bool;`
- `export fn isMatch(path: string, pattern: string): bool;`
- `export fn isMatchAny(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn scanBase(pattern: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`picomatch@1.0.0`)，许可证：MIT
