[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# target-lexicon

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap LLVM/Rust/Zig target triple parser and platform metadata resolver (arch, vendor, os, env, pointer width, endianness, executable suffix)

## 安装与导入

```bash
cap add target-lexicon
```

```cp
import { parseArch } from "target-lexicon";
```

## 核心导出 API

- `export enum ArchKind`
- `export enum OsKind`
- `export enum EnvKind`
- `export fn parseArch(s: string): ArchKind;`
- `export fn parseOs(s: string): OsKind;`
- `export fn parseEnv(s: string): EnvKind;`
- `export fn pointerWidthFor(a: ArchKind): u8;`
- `export fn isLittleEndianFor(a: ArchKind, raw_arch: string): bool;`
- `export fn parseTargetTriple(raw: string): TargetTriple;`
- `export fn exeSuffix(t: TargetTriple): string;`
- `export fn dylibSuffix(t: TargetTriple): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`target-lexicon@1.0.0`)，许可证：Apache-2.0
