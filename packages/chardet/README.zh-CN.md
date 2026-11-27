[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# chardet

Zero-allocation character encoding detector (BOM, UTF-8, UTF-16/32, ASCII, Shift-JIS, GBK, Windows-1252, ISO-8859-1)

| 元信息 | 值 |
| --- | --- |
| 包名 | `chardet` |
| 版本 | `2.1.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add chardet
```

```cp
import {
    DetectedCharset,
    CharsetMatch,
    charsetName,
    chdScoreShiftJis,
    chdScoreGbk,
    detect,
    detectName,
} from "chardet";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DetectedCharset` | `Utf8, Utf8Bom, Utf16Le, Utf16Be, Utf32Le, Utf32Be, Ascii, Windows1252 ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CharsetMatch` | `class` | 字段: `charset: DetectedCharset, name: string, confidence: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `charsetName` | `(cs: DetectedCharset): string` | 调用 `charsetName` 并返回 `string`。 |
| `chdScoreShiftJis` | `(data: &[]u8): u8` | 调用 `chdScoreShiftJis` 并返回 `u8`。 |
| `chdScoreGbk` | `(data: &[]u8): u8` | 调用 `chdScoreGbk` 并返回 `u8`。 |
| `detect` | `(data: &[]u8): CharsetMatch` | 调用 `detect` 并返回 `CharsetMatch`。 |
| `detectName` | `(data: &[]u8): string` | 调用 `detectName` 并返回 `string`。 |

## 最小调用示例

```cp
import { charsetName } from "chardet";

pub fn main() {
    let result = charsetName(/* cs: DetectedCharset */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
