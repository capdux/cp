[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# glob-to-regexp

Zero-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter with extended & globstar modes in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-to-regexp` ([https://github.com/fitzgen/glob-to-regexp](https://github.com/fitzgen/glob-to-regexp))（原库名 `glob-to-regexp` → CP 包名 `glob-to-regexp`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `glob-to-regexp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add glob-to-regexp
```

```cp
import {
    GlobOptions,
    pushByte,
    pushSlice,
    isEscapeMeta,
    globToRegex,
    globToRegexDefault,
} from "glob-to-regexp";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GlobOptions` | `class` | 字段: `extended: bool, globstar: bool, anchored: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | 写入或更新状态数据。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | 写入或更新状态数据。 |
| `isEscapeMeta` | `(b: u8): bool` | 执行 `isEscapeMeta` 条件判定，返回 `bool`。 |
| `globToRegex` | `(glob: &[]u8, opts: GlobOptions, dst: []u8): i64` | 调用 `globToRegex` 并返回 `i64`。 |
| `globToRegexDefault` | `(glob: &[]u8, dst: []u8): i64` | 调用 `globToRegexDefault` 并返回 `i64`。 |

## 最小调用示例

```cp
import { isEscapeMeta } from "glob-to-regexp";

pub fn main() {
    let result = isEscapeMeta(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
