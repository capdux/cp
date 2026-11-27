[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# target-lexicon

Zero-heap LLVM/Rust/Zig target triple parser and platform metadata resolver (arch, vendor, os, env, pointer width, endianness, executable suffix)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:target-lexicon` ([https://github.com/bytecodealliance/target-lexicon](https://github.com/bytecodealliance/target-lexicon))（原库名 `target-lexicon` → CP 包名 `target-lexicon`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `target-lexicon` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add target-lexicon
```

```cp
import {
    ArchKind,
    OsKind,
    EnvKind,
    TargetTriple,
    parseArch,
    parseOs,
    parseEnv,
    pointerWidthFor,
} from "target-lexicon";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ArchKind` | `X86_64, Aarch64, X86, Arm, Riscv64, Riscv32, Wasm32, Wasm64 ...` |
| `enum` | `OsKind` | `Linux, Darwin, Windows, FreeBsd, OpenBsd, NetBsd, Android, Ios ...` |
| `enum` | `EnvKind` | `Gnu, Musl, Msvc, Eabi, Eabihf, None, Unknown` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TargetTriple` | `class` | 字段: `valid: bool, raw_arch: string, raw_vendor: string, raw_os: string, raw_env: string, arch: ArchKind, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseArch` | `(s: string): ArchKind` | 通过 `parseArch` 解析输入数据，返回 `ArchKind`。 |
| `parseOs` | `(s: string): OsKind` | 通过 `parseOs` 解析输入数据，返回 `OsKind`。 |
| `parseEnv` | `(s: string): EnvKind` | 通过 `parseEnv` 解析输入数据，返回 `EnvKind`。 |
| `pointerWidthFor` | `(a: ArchKind): u8` | 调用 `pointerWidthFor` 并返回 `u8`。 |
| `isLittleEndianFor` | `(a: ArchKind, raw_arch: string): bool` | 执行 `isLittleEndianFor` 条件判定，返回 `bool`。 |
| `parseTargetTriple` | `(raw: string): TargetTriple` | 通过 `parseTargetTriple` 解析输入数据，返回 `TargetTriple`。 |
| `exeSuffix` | `(t: TargetTriple): string` | 调用 `exeSuffix` 并返回 `string`。 |
| `dylibSuffix` | `(t: TargetTriple): string` | 调用 `dylibSuffix` 并返回 `string`。 |

## 最小调用示例

```cp
import { parseArch } from "target-lexicon";

pub fn main() {
    let result = parseArch("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
