[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# path-clean

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap lexical POSIX path cleaner following Go filepath.Clean / Rust path-clean semantics in pure CP

## 安装与导入

```bash
cap add path-clean
```

```cp
import { pclByteAt } from "path-clean";
```

## 核心导出 API

- `export fn pclByteAt(a: string, b: string, has_sep: bool, idx: usize): u8;`
- `export fn pclCleanTwo(a: string, b: string, has_sep: bool, out: []u8): usize;`
- `export fn cleanPath(path: string, out: []u8): usize;`
- `export fn cleanJoin(a: string, b: string, out: []u8): usize;`
- `export fn isClean(path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`path-clean@1.0.0`)，许可证：MIT OR Apache-2.0
