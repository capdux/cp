[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# serve-static

Zero-heap HTTP static file request resolver, dotfile policy evaluator, Cache-Control builder, and MIME/ETag helper (clean-room port of npm serve-static)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:serve-static` ([https://github.com/expressjs/serve-static](https://github.com/expressjs/serve-static))

| 元信息 | 值 |
| --- | --- |
| 包名 | `serve-static` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add serve-static
```

```cp
import {
    ServeStaticOptions,
    StaticResolution,
    sstPush,
    sstIsGetOrHead,
    hasDotfileSegment,
    resolveStaticRequest,
    formatCacheControl,
    formatStaticEtag,
} from "serve-static";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ServeStaticOptions` | `class` | 字段: `index_file: string, dotfiles: u8, max_age_secs: u64, immutable: bool, redirect_dirs: bool` · 方法: `defaultOpts` |
| `StaticResolution` | `class` | 字段: `status: u16, rel_path_len: usize, mime_type: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sstPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `sstPush` 并返回 `usize`。 |
| `sstIsGetOrHead` | `(method: string): bool` | 调用 `sstIsGetOrHead` 并返回 `bool`。 |
| `hasDotfileSegment` | `(pathname: string): bool` | 执行 `hasDotfileSegment` 条件判定，返回 `bool`。 |
| `resolveStaticRequest` | `(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution` | 调用 `resolveStaticRequest` 并返回 `StaticResolution`。 |
| `formatCacheControl` | `(max_age_secs: u64, immutable: bool, out: []u8): usize` | 通过 `formatCacheControl` 编码或格式化输出，返回 `usize`。 |
| `formatStaticEtag` | `(size_bytes: u64, mtime_ms: u64, out: []u8): usize` | 通过 `formatStaticEtag` 编码或格式化输出，返回 `usize`。 |
| `formatStaticContentType` | `(file_path: string, out: []u8): usize` | 通过 `formatStaticContentType` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatStaticContentType } from "serve-static";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatStaticContentType("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
