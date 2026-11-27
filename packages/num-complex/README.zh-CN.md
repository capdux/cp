[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-complex

Zero-allocation Complex64 arithmetic, Smith overflow-safe division, polar coordinates, Euler exp, principal sqrt, and formatting (Rust num-complex parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-complex` ([https://github.com/rust-num/num-complex](https://github.com/rust-num/num-complex))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-complex` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-complex
```

```cp
import {
    Complex64,
    fabs,
    pushByte,
    pushU64,
    pushAbsFloat,
    formatComplex,
} from "num-complex";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Complex64` | `class` | 字段: `re: f64, im: f64` · 方法: `new, zero, one, i, fromPolar, add, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fabs` | `(x: f64): f64` | 调用 `fabs` 并返回 `f64`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `pushU64` | `(out: []u8, pos: usize, val: u64): usize` | 写入或更新状态数据。 |
| `pushAbsFloat` | `(out: []u8, pos: usize, v: f64): usize` | 写入或更新状态数据。 |
| `formatComplex` | `(z: Complex64, out: []u8): string` | 通过 `formatComplex` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { formatComplex } from "num-complex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatComplex(/* z: Complex64 */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
