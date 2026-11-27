[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/expat

Stream-oriented XML 1.0 SAX parser (libexpat 2.6.4) native C binding for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/expat` |
| 版本 | `2.6.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/expat
```

```cp
import {
    ExpatError,
    cp_expat_version,
    cp_expat_count_elements,
    versionString,
    countElements,
} from "@cap/expat";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `ExpatError` | `InvalidInput, ParserInitFailed, ParseFailed` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_expat_version` | `(): [*:0]&u8!` | 调用 `cp_expat_version` 并返回 `[*:0]&u8!`。 |
| `cp_expat_count_elements` | `(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32` | 调用 `cp_expat_count_elements` 并返回 `i32`。 |
| `versionString` | `(): string` | 调用 `versionString` 并返回 `string`。 |
| `countElements` | `(xml: &[]u8): !usize` | 调用 `countElements` 并返回 `!usize`。 |

## 最小调用示例

```cp
import { versionString } from "@cap/expat";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
