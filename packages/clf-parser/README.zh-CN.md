[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# clf-parser

Zero-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clf-parser` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `clf-parser` → CP 包名 `clf-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `clf-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add clf-parser
```

```cp
import {
    ClfRecord,
    invalidClf,
    isClfSpace,
    isLineEnd,
    parseClf,
    pushSlice,
    pushU64,
    formatClf,
} from "clf-parser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ClfRecord` | `class` | 字段: `host_start: usize, host_len: usize, user_start: usize, user_len: usize, time_start: usize, time_len: usize, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidClf` | `(): ClfRecord` | 调用 `invalidClf` 并返回 `ClfRecord`。 |
| `isClfSpace` | `(b: u8): bool` | 执行 `isClfSpace` 条件判定，返回 `bool`。 |
| `isLineEnd` | `(b: u8): bool` | 执行 `isLineEnd` 条件判定，返回 `bool`。 |
| `parseClf` | `(line: &[]u8): ClfRecord` | 通过 `parseClf` 解析输入数据，返回 `ClfRecord`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | 写入或更新状态数据。 |
| `pushU64` | `(dst: []u8, pos: usize, val: u64): i64` | 写入或更新状态数据。 |
| `formatClf` | `(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64` | 通过 `formatClf` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { isClfSpace } from "clf-parser";

pub fn main() {
    let result = isClfSpace(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
