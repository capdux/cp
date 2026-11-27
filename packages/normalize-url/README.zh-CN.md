[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# normalize-url

Zero-allocation URL normalization utility (npm normalize-url parity) composing prepend-http for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-url` ([https://github.com/sindresorhus/normalize-url](https://github.com/sindresorhus/normalize-url))

| 元信息 | 值 |
| --- | --- |
| 包名 | `normalize-url` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add normalize-url
```

```cp
import {
    NormalizeUrlOptions,
    NurlSegment,
    defaultNormalizeUrlOptions,
    nurlWriteByte,
    nurlWriteLower,
    nurlFindSchemeSep,
    nurlIsDefaultPort,
    nurlNormalizePath,
} from "normalize-url";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `NormalizeUrlOptions` | `class` | 字段: `default_https: bool, strip_www: bool, strip_hash: bool, remove_trailing_slash: bool, remove_default_port: bool, normalize_protocol: bool` |
| `NurlSegment` | `class` | 字段: `start: usize, end: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultNormalizeUrlOptions` | `(): NormalizeUrlOptions` | 调用 `defaultNormalizeUrlOptions` 并返回 `NormalizeUrlOptions`。 |
| `nurlWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `nurlWriteByte` 并返回 `usize`。 |
| `nurlWriteLower` | `(out: []u8, pos: usize, s: string): usize` | 调用 `nurlWriteLower` 并返回 `usize`。 |
| `nurlFindSchemeSep` | `(s: string): usize` | 调用 `nurlFindSchemeSep` 并返回 `usize`。 |
| `nurlIsDefaultPort` | `(scheme: string, port: string): bool` | 调用 `nurlIsDefaultPort` 并返回 `bool`。 |
| `nurlNormalizePath` | `(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize` | 调用 `nurlNormalizePath` 并返回 `usize`。 |
| `normalizeUrlWithOptions` | `(url: string, opts: NormalizeUrlOptions, out: []u8): usize` | 调用 `normalizeUrlWithOptions` 并返回 `usize`。 |
| `normalizeUrl` | `(url: string, out: []u8): usize` | 调用 `normalizeUrl` 并返回 `usize`。 |

## 最小调用示例

```cp
import { nurlWriteLower } from "normalize-url";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nurlWriteLower(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
