[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# micromatch

Zero-allocation glob matching suite (npm micromatch v4 parity) with brace range expansion, multi-pattern filtering, negation, and contains matching for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:micromatch` ([https://github.com/micromatch/micromatch](https://github.com/micromatch/micromatch))

| 元信息 | 值 |
| --- | --- |
| 包名 | `micromatch` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add micromatch
```

```cp
import {
    mmHasBrace,
    mmCountLeadingNeg,
    mmMatchPositiveExpanded,
    mmWriteWrapped,
    mmContainsSinglePositive,
    mmIsMatch,
    mmAll,
    mmAny,
} from "micromatch";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mmHasBrace` | `(s: string): bool` | 调用 `mmHasBrace` 并返回 `bool`。 |
| `mmCountLeadingNeg` | `(pat: string): usize` | 调用 `mmCountLeadingNeg` 并返回 `usize`。 |
| `mmMatchPositiveExpanded` | `(path: string, inner: string, opts: PicoOptions): bool` | 调用 `mmMatchPositiveExpanded` 并返回 `bool`。 |
| `mmWriteWrapped` | `(dst: []u8, prefix: string, mid: string, suffix: string): usize` | 调用 `mmWriteWrapped` 并返回 `usize`。 |
| `mmContainsSinglePositive` | `(path: string, pat: string, opts: PicoOptions): bool` | 调用 `mmContainsSinglePositive` 并返回 `bool`。 |
| `mmIsMatch` | `(path: string, pattern: string, opts: PicoOptions): bool` | 调用 `mmIsMatch` 并返回 `bool`。 |
| `mmAll` | `(path: string, patterns: &[]string, opts: PicoOptions): bool` | 调用 `mmAll` 并返回 `bool`。 |
| `mmAny` | `(path: string, patterns: &[]string, opts: PicoOptions): bool` | 调用 `mmAny` 并返回 `bool`。 |
| `mmSome` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool` | 调用 `mmSome` 并返回 `bool`。 |
| `mmEvery` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool` | 调用 `mmEvery` 并返回 `bool`。 |
| `mmFilter` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize` | 调用 `mmFilter` 并返回 `usize`。 |
| `mmNot` | `(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize` | 调用 `mmNot` 并返回 `usize`。 |
| `mmContains` | `(path: string, pattern: string, opts: PicoOptions): bool` | 调用 `mmContains` 并返回 `bool`。 |

## 最小调用示例

```cp
import { mmHasBrace } from "micromatch";

pub fn main() {
    let result = mmHasBrace("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
