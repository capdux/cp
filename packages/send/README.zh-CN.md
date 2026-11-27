[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# send

Zero-heap pure CP static file transfer planner with traversal protection, dotfiles policy, ETag/304, Range/206, and redirect URL encoding (clean-room MIT implementation of npm send v1)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:send` ([https://github.com/pillarjs/send](https://github.com/pillarjs/send))

| 元信息 | 值 |
| --- | --- |
| 包名 | `send` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add send
```

```cp
import {
    SendOptions,
    SendPlan,
    sndStrEq,
    sndIsWs,
    sndTrimWs,
    sndHasMaliciousPath,
    sndContainsDotfile,
    sndIfNoneMatchMatches,
} from "send";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SendOptions` | `class` | 字段: `root: string, dotfiles: u8, etag: bool, accept_ranges: bool, max_age_secs: u64, immutable: bool` · 方法: `defaults` |
| `SendPlan` | `class` | 字段: `status: u16, content_type: string, is_partial: bool, range_start: u64, range_end: u64, content_length: u64, +1 项` · 方法: `errorPlan` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sndStrEq` | `(a: string, b: string): bool` | 调用 `sndStrEq` 并返回 `bool`。 |
| `sndIsWs` | `(b: u8): bool` | 调用 `sndIsWs` 并返回 `bool`。 |
| `sndTrimWs` | `(s: string): string` | 调用 `sndTrimWs` 并返回 `string`。 |
| `sndHasMaliciousPath` | `(req_path: string): bool` | 调用 `sndHasMaliciousPath` 并返回 `bool`。 |
| `sndContainsDotfile` | `(req_path: string): bool` | 调用 `sndContainsDotfile` 并返回 `bool`。 |
| `sndIfNoneMatchMatches` | `(if_none_match: string, current_etag: string): bool` | 调用 `sndIfNoneMatchMatches` 并返回 `bool`。 |
| `planSendFile` | `(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan` | 调用 `planSendFile` 并返回 `SendPlan`。 |
| `formatRedirectUrl` | `(path: string, out: []u8): usize` | 通过 `formatRedirectUrl` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatRedirectUrl } from "send";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatRedirectUrl("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
