[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# anymatch

Zero-heap file path & glob matcher with negation support, composing normalize-path and is-glob in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:anymatch` ([https://github.com/micromatch/anymatch](https://github.com/micromatch/anymatch))

| 元信息 | 值 |
| --- | --- |
| 包名 | `anymatch` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add anymatch
```

```cp
import {
    startsWithDirPrefix,
    globMatchAt,
    matchPattern,
    anymatchIndex,
    anymatch,
    countGlobPatterns,
} from "anymatch";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `startsWithDirPrefix` | `(path: &[]u8, prefix: &[]u8): bool` | 调用 `startsWithDirPrefix` 并返回 `bool`。 |
| `globMatchAt` | `(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool` | 调用 `globMatchAt` 并返回 `bool`。 |
| `matchPattern` | `(pattern: string, test_path: string): bool` | 调用 `matchPattern` 并返回 `bool`。 |
| `anymatchIndex` | `(patterns: &[]string, test_path: string): i64` | 调用 `anymatchIndex` 并返回 `i64`。 |
| `anymatch` | `(patterns: &[]string, test_path: string): bool` | 调用 `anymatch` 并返回 `bool`。 |
| `countGlobPatterns` | `(patterns: &[]string): usize` | 调用 `countGlobPatterns` 并返回 `usize`。 |

## 最小调用示例

```cp
import { matchPattern } from "anymatch";

pub fn main() {
    let result = matchPattern("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
