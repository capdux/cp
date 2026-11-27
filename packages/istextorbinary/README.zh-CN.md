[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# istextorbinary

Zero-allocation WHATWG MIME Sniffing Standard binary/text detector and HTML/XML/JSON/SVG content sniffer for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:istextorbinary` ([https://github.com/bevry/istextorbinary](https://github.com/bevry/istextorbinary)) / `crates.io:content_inspector` ([https://github.com/sharkdp/content_inspector](https://github.com/sharkdp/content_inspector))（原库名 `istextorbinary` → CP 包名 `istextorbinary`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `istextorbinary` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add istextorbinary
```

```cp
import {
    msToLower,
    msIsSpace,
    msIsTagTerminator,
    msStartsWithIgnoreCase,
    msMatchTag,
    isBinary,
    isText,
    sniffContentType,
} from "istextorbinary";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `msToLower` | `(c: u8): u8` | 调用 `msToLower` 并返回 `u8`。 |
| `msIsSpace` | `(c: u8): bool` | 调用 `msIsSpace` 并返回 `bool`。 |
| `msIsTagTerminator` | `(c: u8): bool` | 调用 `msIsTagTerminator` 并返回 `bool`。 |
| `msStartsWithIgnoreCase` | `(s: &[]u8, prefix: &[]u8): bool` | 调用 `msStartsWithIgnoreCase` 并返回 `bool`。 |
| `msMatchTag` | `(s: &[]u8, tagPrefix: &[]u8): bool` | 调用 `msMatchTag` 并返回 `bool`。 |
| `isBinary` | `(data: &[]u8): bool` | 执行 `isBinary` 条件判定，返回 `bool`。 |
| `isText` | `(data: &[]u8): bool` | 执行 `isText` 条件判定，返回 `bool`。 |
| `sniffContentType` | `(data: &[]u8): u8` | 调用 `sniffContentType` 并返回 `u8`。 |
| `contentTypeToMime` | `(ct: u8): string` | 调用 `contentTypeToMime` 并返回 `string`。 |
| `sniffMime` | `(data: &[]u8): string` | 调用 `sniffMime` 并返回 `string`。 |

## 最小调用示例

```cp
import { isBinary } from "istextorbinary";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isBinary(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
