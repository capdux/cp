[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# micromark-util-sanitize-uri

Zero-allocation safe URI sanitizer and RFC 3986 percent-encoder blocking unsafe schemes

| 元信息 | 值 |
| --- | --- |
| 包名 | `micromark-util-sanitize-uri` |
| 版本 | `2.0.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add micromark-util-sanitize-uri
```

```cp
import { surIsSafeUriByte, isSafeUri, sanitizeUri } from "micromark-util-sanitize-uri";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `surIsSafeUriByte` | `(b: u8): bool` | 调用 `surIsSafeUriByte` 并返回 `bool`。 |
| `isSafeUri` | `(uri: string): bool` | 执行 `isSafeUri` 条件判定，返回 `bool`。 |
| `sanitizeUri` | `(uri: string, out: []u8): usize` | 调用 `sanitizeUri` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isSafeUri } from "micromark-util-sanitize-uri";

pub fn main() {
    let result = isSafeUri("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
