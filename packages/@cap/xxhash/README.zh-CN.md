[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/xxhash

xxHash 0.8.3 extremely fast non-cryptographic hash algorithm (XXH32/XXH64/XXH3) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/xxhash` |
| 版本 | `0.8.3` |
| 许可证 | `BSD-2-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/xxhash
```

```cp
import {
    XXH_versionNumber,
    XXH32,
    XXH64,
    XXH3_64bits,
    versionNumber,
    xxh32,
    xxh64,
    xxh3_64bits,
} from "@cap/xxhash";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `XXH_versionNumber` | `(): u32` | 调用 `XXH_versionNumber` 并返回 `u32`。 |
| `XXH32` | `(input: [*]&u8!, length: usize, seed: u32): u32` | 调用 `XXH32` 并返回 `u32`。 |
| `XXH64` | `(input: [*]&u8!, length: usize, seed: u64): u64` | 调用 `XXH64` 并返回 `u64`。 |
| `XXH3_64bits` | `(input: [*]&u8!, length: usize): u64` | 调用 `XXH3_64bits` 并返回 `u64`。 |
| `versionNumber` | `(): u32` | 调用 `versionNumber` 并返回 `u32`。 |
| `xxh32` | `(data: &[]u8, seed: u32): u32` | 调用 `xxh32` 并返回 `u32`。 |
| `xxh64` | `(data: &[]u8, seed: u64): u64` | 调用 `xxh64` 并返回 `u64`。 |
| `xxh3_64bits` | `(data: &[]u8): u64` | 调用 `xxh3_64bits` 并返回 `u64`。 |

## 最小调用示例

```cp
import { versionNumber } from "@cap/xxhash";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
