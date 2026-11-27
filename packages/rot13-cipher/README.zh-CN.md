[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rot13-cipher

Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rot13-cipher` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:rot13` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `rot13-cipher` → CP 包名 `rot13-cipher`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rot13-cipher` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rot13-cipher
```

```cp
import {
    shiftLetter,
    rot13,
    rot13InPlace,
    rot47,
    caesar,
    atbash,
} from "rot13-cipher";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `shiftLetter` | `(c: u8, norm_shift: u8): u8` | 调用 `shiftLetter` 并返回 `u8`。 |
| `rot13` | `(input: string, out: []u8): usize` | 调用 `rot13` 并返回 `usize`。 |
| `rot13InPlace` | `(buf: []u8)` | 就地执行 `rot13InPlace` 操作。 |
| `rot47` | `(input: string, out: []u8): usize` | 调用 `rot47` 并返回 `usize`。 |
| `caesar` | `(input: string, shift: i32, out: []u8): usize` | 调用 `caesar` 并返回 `usize`。 |
| `atbash` | `(input: string, out: []u8): usize` | 调用 `atbash` 并返回 `usize`。 |

## 最小调用示例

```cp
import { rot13 } from "rot13-cipher";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = rot13("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
