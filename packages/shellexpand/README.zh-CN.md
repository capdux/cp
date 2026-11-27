[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shellexpand

Zero-allocation shell-like tilde (~) and environment variable ($VAR, ${VAR}, ${VAR:-default}) expansion (clean-room MIT implementation of crates.io shellexpand)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:shellexpand` ([https://gitlab.com/ijackson/rust-shellexpand](https://gitlab.com/ijackson/rust-shellexpand)) | 许可：`MIT OR Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `shellexpand` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shellexpand
```

```cp
import {
    EnvEntry,
    sexpIsVarStart,
    sexpIsVarChar,
    sexpStrEq,
    sexpLookup,
    sexpAppend,
    tildeWithHome,
    envExpand,
} from "shellexpand";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EnvEntry` | `class` | 字段: `key: string, value: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sexpIsVarStart` | `(b: u8): bool` | 调用 `sexpIsVarStart` 并返回 `bool`。 |
| `sexpIsVarChar` | `(b: u8): bool` | 调用 `sexpIsVarChar` 并返回 `bool`。 |
| `sexpStrEq` | `(a: string, b: string): bool` | 调用 `sexpStrEq` 并返回 `bool`。 |
| `sexpLookup` | `(vars: &[]EnvEntry, key: string): string?` | 调用 `sexpLookup` 并返回 `string?`。 |
| `sexpAppend` | `(out: []u8, pos: usize, s: string): usize` | 调用 `sexpAppend` 并返回 `usize`。 |
| `tildeWithHome` | `(input: string, home_dir: string, out: []u8): usize` | 调用 `tildeWithHome` 并返回 `usize`。 |
| `envExpand` | `(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize` | 调用 `envExpand` 并返回 `usize`。 |
| `fullExpand` | `(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize` | 调用 `fullExpand` 并返回 `usize`。 |

## 最小调用示例

```cp
import { sexpStrEq } from "shellexpand";

pub fn main() {
    let result = sexpStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
