[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# micromatch

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation glob matching suite (npm micromatch v4 parity) with brace range expansion, multi-pattern filtering, negation, and contains matching for CP

## 安装与导入

```bash
cap add micromatch
```

```cp
import { mmIsMatch } from "micromatch";
```

## 核心导出 API

- `export fn mmIsMatch(path: string, pattern: string, opts: PicoOptions): bool;`
- `export fn mmAll(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmAny(path: string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmSome(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmEvery(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;`
- `export fn mmFilter(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;`
- `export fn mmNot(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;`
- `export fn mmContains(path: string, pattern: string, opts: PicoOptions): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`micromatch@1.0.0`)，许可证：MIT
