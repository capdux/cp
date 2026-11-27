[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# stylis

Zero-heap CSS minifier and nested selector preprocessor in pure CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `stylis` |
| 版本 | `4.3.6` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add stylis
```

```cp
import {
    stylIsWs,
    stylPushByte,
    stylIsStripAfter,
    stylIsStripBefore,
    minifyCss,
    stylExpandSingle,
    expandNestedSelector,
    stylSkipCommentOrQuote,
} from "stylis";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `stylIsWs` | `(c: u8): bool` | 调用 `stylIsWs` 并返回 `bool`。 |
| `stylPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `stylPushByte` 并返回 `usize`。 |
| `stylIsStripAfter` | `(c: u8): bool` | 调用 `stylIsStripAfter` 并返回 `bool`。 |
| `stylIsStripBefore` | `(c: u8): bool` | 调用 `stylIsStripBefore` 并返回 `bool`。 |
| `minifyCss` | `(css: string, out: []u8): usize` | 调用 `minifyCss` 并返回 `usize`。 |
| `stylExpandSingle` | `(parent: string, child: string, out: []u8, start_pos: usize): usize` | 调用 `stylExpandSingle` 并返回 `usize`。 |
| `expandNestedSelector` | `(parent: string, child: string, out: []u8): usize` | 调用 `expandNestedSelector` 并返回 `usize`。 |
| `stylSkipCommentOrQuote` | `(s: string, idx: usize): usize` | 调用 `stylSkipCommentOrQuote` 并返回 `usize`。 |
| `compileNestedCss` | `(parent_sel: string, body_css: string, out: []u8): usize` | 调用 `compileNestedCss` 并返回 `usize`。 |

## 最小调用示例

```cp
import { minifyCss } from "stylis";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = minifyCss("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
