[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# short-uuid

Zero-allocation 128-bit UUID to 22-char Flickr Base58 / Base62 / Cookie Base90 lossless short ID translator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:short-uuid` ([https://github.com/oculus42/short-uuid](https://github.com/oculus42/short-uuid))

| 元信息 | 值 |
| --- | --- |
| 包名 | `short-uuid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add short-uuid
```

```cp
import {
    parseHexNibble,
    hexNibbleLower,
    uuidToBytes,
    bytesToUuid,
    validateAlphabet,
    findInAlphabet,
    maxEncodedLen,
    shortenUuidWith,
} from "short-uuid";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseHexNibble` | `(c: u8): u8?` | 通过 `parseHexNibble` 解析输入数据，返回 `u8?`。 |
| `hexNibbleLower` | `(v: u8): u8` | 调用 `hexNibbleLower` 并返回 `u8`。 |
| `uuidToBytes` | `(uuid: string, out16: []u8): bool` | 调用 `uuidToBytes` 并返回 `bool`。 |
| `bytesToUuid` | `(bytes16: []u8, out36: []u8): string` | 调用 `bytesToUuid` 并返回 `string`。 |
| `validateAlphabet` | `(alphabet: string): bool` | 校验输入合法性，返回 `bool`。 |
| `findInAlphabet` | `(alphabet: string, c: u8): u32?` | 查询或读取目标数据，返回 `u32?`。 |
| `maxEncodedLen` | `(base: u32): usize` | 调用 `maxEncodedLen` 并返回 `usize`。 |
| `shortenUuidWith` | `(uuid: string, alphabet: string, out: []u8): string?` | 调用 `shortenUuidWith` 并返回 `string?`。 |
| `expandUuidWith` | `(short_id: string, alphabet: string, out36: []u8): string?` | 调用 `expandUuidWith` 并返回 `string?`。 |
| `shortenUuid` | `(uuid: string, out: []u8): string?` | 调用 `shortenUuid` 并返回 `string?`。 |
| `expandUuid` | `(short_id: string, out36: []u8): string?` | 调用 `expandUuid` 并返回 `string?`。 |

## 最小调用示例

```cp
import { validateAlphabet } from "short-uuid";

pub fn main() {
    let result = validateAlphabet("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
