[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ufo

Zero-heap URL protocol detection, slash normalization, and safe URL joining utilities

| 元信息 | 值 |
| --- | --- |
| 包名 | `ufo` |
| 版本 | `1.5.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ufo
```

```cp
import {
    ufoIsAlpha,
    ufoIsSchemeChar,
    ufoProtocolEnd,
    ufoCopyBytes,
    hasProtocol,
    isRelative,
    withoutLeadingSlash,
    withoutTrailingSlash,
} from "ufo";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ufoIsAlpha` | `(c: u8): bool` | 调用 `ufoIsAlpha` 并返回 `bool`。 |
| `ufoIsSchemeChar` | `(c: u8): bool` | 调用 `ufoIsSchemeChar` 并返回 `bool`。 |
| `ufoProtocolEnd` | `(url: string): usize` | 调用 `ufoProtocolEnd` 并返回 `usize`。 |
| `ufoCopyBytes` | `(src: string, out: []u8, offset: usize): usize` | 调用 `ufoCopyBytes` 并返回 `usize`。 |
| `hasProtocol` | `(url: string): bool` | 执行 `hasProtocol` 条件判定，返回 `bool`。 |
| `isRelative` | `(url: string): bool` | 执行 `isRelative` 条件判定，返回 `bool`。 |
| `withoutLeadingSlash` | `(s: string): string` | 调用 `withoutLeadingSlash` 并返回 `string`。 |
| `withoutTrailingSlash` | `(s: string): string` | 调用 `withoutTrailingSlash` 并返回 `string`。 |
| `withLeadingSlash` | `(s: string, out: []u8): usize` | 调用 `withLeadingSlash` 并返回 `usize`。 |
| `withTrailingSlash` | `(s: string, out: []u8): usize` | 调用 `withTrailingSlash` 并返回 `usize`。 |
| `cleanDoubleSlashes` | `(url: string, out: []u8): usize` | 调用 `cleanDoubleSlashes` 并返回 `usize`。 |
| `joinUrl` | `(base: string, segment: string, out: []u8): usize` | 调用 `joinUrl` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isRelative } from "ufo";

pub fn main() {
    let result = isRelative("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
