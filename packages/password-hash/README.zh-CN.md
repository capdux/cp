[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# password-hash

Zero-allocation pure CP PHC string format parser, serializer, and constant-time comparator (RustCrypto password-hash parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:password-hash` ([https://github.com/RustCrypto/password-hashes](https://github.com/RustCrypto/password-hashes))

| 元信息 | 值 |
| --- | --- |
| 包名 | `password-hash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add password-hash
```

```cp
import {
    PhcParam,
    PhcHash,
    phchEmptyHash,
    phchIsLowerAlnumHyphen,
    phchIsParamValueChar,
    phchIsB64Char,
    phchIsValidAlgo,
    phchIsValidParamKey,
} from "password-hash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PhcParam` | `class` | 字段: `key: string, value: string` |
| `PhcHash` | `class` | 字段: `algorithm: string, version: u32, has_version: bool, params: [8]PhcParam, param_count: usize, salt: string, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `phchEmptyHash` | `(): PhcHash` | 调用 `phchEmptyHash` 并返回 `PhcHash`。 |
| `phchIsLowerAlnumHyphen` | `(c: u8): bool` | 调用 `phchIsLowerAlnumHyphen` 并返回 `bool`。 |
| `phchIsParamValueChar` | `(c: u8): bool` | 调用 `phchIsParamValueChar` 并返回 `bool`。 |
| `phchIsB64Char` | `(c: u8): bool` | 调用 `phchIsB64Char` 并返回 `bool`。 |
| `phchIsValidAlgo` | `(s: string): bool` | 调用 `phchIsValidAlgo` 并返回 `bool`。 |
| `phchIsValidParamKey` | `(s: string): bool` | 调用 `phchIsValidParamKey` 并返回 `bool`。 |
| `phchIsValidParamValue` | `(s: string): bool` | 调用 `phchIsValidParamValue` 并返回 `bool`。 |
| `phchIsValidSalt` | `(s: string): bool` | 调用 `phchIsValidSalt` 并返回 `bool`。 |
| `phchIsValidHash` | `(s: string): bool` | 调用 `phchIsValidHash` 并返回 `bool`。 |
| `phchContainsByte` | `(s: string, target: u8): bool` | 调用 `phchContainsByte` 并返回 `bool`。 |
| `phchParseU32` | `(s: string): i64` | 调用 `phchParseU32` 并返回 `i64`。 |
| `phchStrEq` | `(a: string, b: string): bool` | 调用 `phchStrEq` 并返回 `bool`。 |
| `phchAppendBytes` | `(out: []u8, pos: usize, src: string): usize` | 调用 `phchAppendBytes` 并返回 `usize`。 |
| `phchAppendU32` | `(out: []u8, pos: usize, val: u32): usize` | 调用 `phchAppendU32` 并返回 `usize`。 |
| `parsePhc` | `(phc_str: string): PhcHash` | 通过 `parsePhc` 解析输入数据，返回 `PhcHash`。 |
| `formatPhc` | `(h: PhcHash, out: []u8): usize` | 通过 `formatPhc` 编码或格式化输出，返回 `usize`。 |
| `getPhcParam` | `(h: PhcHash, key: string): string?` | 查询或读取目标数据，返回 `string?`。 |
| `ctEqual` | `(a: string, b: string): bool` | 调用 `ctEqual` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parsePhc } from "password-hash";

pub fn main() {
    let result = parsePhc("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
