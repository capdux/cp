[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# salsa20

Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce) verified against official Bernstein/eSTREAM vectors

| 元信息 | 值 |
| --- | --- |
| 包名 | `salsa20` |
| 版本 | `0.10.2` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add salsa20
```

```cp
import {
    s20Rotl,
    s20LoadLe32,
    s20StoreLe32,
    salsa20Block,
    salsa20Apply,
} from "salsa20";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `s20Rotl` | `(v: u32, shift: u5): u32` | 调用 `s20Rotl` 并返回 `u32`。 |
| `s20LoadLe32` | `(src: &[]u8, off: usize): u32` | 调用 `s20LoadLe32` 并返回 `u32`。 |
| `s20StoreLe32` | `(v: u32, dst: []u8, off: usize)` | 就地执行 `s20StoreLe32` 操作。 |
| `salsa20Block` | `(key: &[]u8, nonce: &[]u8, counter: u64, out: []u8)` | 就地执行 `salsa20Block` 操作。 |
| `salsa20Apply` | `(key: &[]u8, nonce: &[]u8, initial_counter: u64, input: &[]u8, out: []u8): usize` | 调用 `salsa20Apply` 并返回 `usize`。 |

## 最小调用示例

```cp
import { s20Rotl } from "salsa20";

pub fn main() {
    let result = s20Rotl(0, /* shift: u5 */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
