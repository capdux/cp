[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# vary

Zero-heap RFC 9110 HTTP Vary header manipulation, deduplication, case-insensitive lookup, token validation, and wildcard collapsing.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:vary` ([https://github.com/jshttp/vary](https://github.com/jshttp/vary))

| 元信息 | 值 |
| --- | --- |
| 包名 | `vary` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add vary
```

```cp
import {
    isWs,
    isTchar,
    isValidFieldName,
    isWildcard,
    hasField,
    headerHasWildcard,
    validateFieldList,
    dstContainsToken,
} from "vary";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `isTchar` | `(c: u8): bool` | 执行 `isTchar` 条件判定，返回 `bool`。 |
| `isValidFieldName` | `(field: &[]u8): bool` | 执行 `isValidFieldName` 条件判定，返回 `bool`。 |
| `isWildcard` | `(tok: &[]u8): bool` | 执行 `isWildcard` 条件判定，返回 `bool`。 |
| `hasField` | `(header: &[]u8, field: &[]u8): bool` | 执行 `hasField` 条件判定，返回 `bool`。 |
| `headerHasWildcard` | `(src: &[]u8): bool` | 调用 `headerHasWildcard` 并返回 `bool`。 |
| `validateFieldList` | `(field_list: &[]u8): bool` | 校验输入合法性，返回 `bool`。 |
| `dstContainsToken` | `(dst: &[]u8, written: usize, tok: &[]u8): bool` | 调用 `dstContainsToken` 并返回 `bool`。 |
| `appendTokens` | `(src: &[]u8, dst: []u8, start_pos: usize): i64` | 调用 `appendTokens` 并返回 `i64`。 |
| `append` | `(header: &[]u8, field: &[]u8, dst: []u8): i64` | 调用 `append` 并返回 `i64`。 |
| `appendMany` | `(header: &[]u8, fields: &[]&[]u8, dst: []u8): i64` | 调用 `appendMany` 并返回 `i64`。 |

## 最小调用示例

```cp
import { isWs } from "vary";

pub fn main() {
    let result = isWs(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
