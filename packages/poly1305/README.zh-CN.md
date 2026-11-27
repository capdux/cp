[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# poly1305

Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator (Donna 26-bit limbs + constant-time verification via subtle-ct)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:poly1305` ([https://github.com/RustCrypto/universal-hashes](https://github.com/RustCrypto/universal-hashes))

| 元信息 | 值 |
| --- | --- |
| 包名 | `poly1305` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add poly1305
```

```cp
import {
    Poly1305,
    p1305ReadU32Le,
    p1305WriteU32Le,
    poly1305Mac,
    poly1305Verify,
} from "poly1305";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Poly1305` | `class` | 字段: `r0: u32, r1: u32, r2: u32, r3: u32, r4: u32, pad0: u32, +10 项` · 方法: `init, processBlock, update, finalize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `p1305ReadU32Le` | `(b: []u8, off: usize): u32` | 调用 `p1305ReadU32Le` 并返回 `u32`。 |
| `p1305WriteU32Le` | `(out: []u8, off: usize, v: u32)` | 就地执行 `p1305WriteU32Le` 操作。 |
| `poly1305Mac` | `(key32: []u8, msg: []u8, out_tag16: []u8): bool` | 调用 `poly1305Mac` 并返回 `bool`。 |
| `poly1305Verify` | `(key32: []u8, msg: []u8, expected_tag16: []u8): bool` | 调用 `poly1305Verify` 并返回 `bool`。 |

## 最小调用示例

```cp
import { p1305ReadU32Le } from "poly1305";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = p1305ReadU32Le(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
