[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# camelcase-keys

Zero-heap streaming JSON object key case converter (camelCase, snake_case, PascalCase) composing camelcase in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:camelcase-keys` ([https://github.com/sindresorhus/camelcase-keys](https://github.com/sindresorhus/camelcase-keys)) / `npm:decamelize-keys` ([https://github.com/sindresorhus/decamelize-keys](https://github.com/sindresorhus/decamelize-keys))

| 元信息 | 值 |
| --- | --- |
| 包名 | `camelcase-keys` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add camelcase-keys
```

```cp
import {
    ckIsSpace,
    ckWriteByte,
    ckWriteSlice,
    ckConvertJsonKeys,
    convertJsonKeysToCamel,
    convertJsonKeysToSnake,
    convertJsonKeysToPascal,
} from "camelcase-keys";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ckIsSpace` | `(ch: u8): bool` | 调用 `ckIsSpace` 并返回 `bool`。 |
| `ckWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `ckWriteByte` 并返回 `usize`。 |
| `ckWriteSlice` | `(out: []u8, pos: usize, src: &[]u8): usize` | 调用 `ckWriteSlice` 并返回 `usize`。 |
| `ckConvertJsonKeys` | `(json: string, mode: u8, out: []u8): usize` | 调用 `ckConvertJsonKeys` 并返回 `usize`。 |
| `convertJsonKeysToCamel` | `(json: string, out: []u8): usize` | 调用 `convertJsonKeysToCamel` 并返回 `usize`。 |
| `convertJsonKeysToSnake` | `(json: string, out: []u8): usize` | 调用 `convertJsonKeysToSnake` 并返回 `usize`。 |
| `convertJsonKeysToPascal` | `(json: string, out: []u8): usize` | 调用 `convertJsonKeysToPascal` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ckConvertJsonKeys } from "camelcase-keys";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ckConvertJsonKeys("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
