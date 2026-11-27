[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bwt

Zero-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bwt` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bwt` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bwt
```

```cp
import {
    BwtResult,
    cmpRotations,
    bwtForward,
    bwtInverse,
    mtfEncode,
    mtfDecode,
} from "bwt";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BwtResult` | `class` | 字段: `primary_index: usize, len: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmpRotations` | `(src: &[]u8, a: usize, b: usize): i32` | 调用 `cmpRotations` 并返回 `i32`。 |
| `bwtForward` | `(src: &[]u8, dst: []u8): BwtResult` | 调用 `bwtForward` 并返回 `BwtResult`。 |
| `bwtInverse` | `(src: &[]u8, primary_index: usize, dst: []u8): bool` | 调用 `bwtInverse` 并返回 `bool`。 |
| `mtfEncode` | `(src: &[]u8, dst: []u8): bool` | 调用 `mtfEncode` 并返回 `bool`。 |
| `mtfDecode` | `(src: &[]u8, dst: []u8): bool` | 调用 `mtfDecode` 并返回 `bool`。 |

## 最小调用示例

```cp
import { cmpRotations } from "bwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cmpRotations(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
