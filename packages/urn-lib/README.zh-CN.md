[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# urn-lib

Zero-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:urn-lib` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:urn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `urn-lib` → CP 包名 `urn-lib`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `urn-lib` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add urn-lib
```

```cp
import {
    Urn,
    invalidUrn,
    urnIsSpace,
    isValidNid,
    parseUrn,
    urnEquivalent,
    pushSlice,
    pushLowerSlice,
} from "urn-lib";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Urn` | `class` | 字段: `nid: string, nss: string, r_component: string, q_component: string, f_component: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidUrn` | `(): Urn` | 调用 `invalidUrn` 并返回 `Urn`。 |
| `urnIsSpace` | `(b: u8): bool` | 调用 `urnIsSpace` 并返回 `bool`。 |
| `isValidNid` | `(nid: string): bool` | 执行 `isValidNid` 条件判定，返回 `bool`。 |
| `parseUrn` | `(input: string): Urn` | 通过 `parseUrn` 解析输入数据，返回 `Urn`。 |
| `urnEquivalent` | `(a: Urn, b: Urn): bool` | 调用 `urnEquivalent` 并返回 `bool`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushLowerSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `formatUrn` | `(u: Urn, out: []u8): usize` | 通过 `formatUrn` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidNid } from "urn-lib";

pub fn main() {
    let result = isValidNid("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
