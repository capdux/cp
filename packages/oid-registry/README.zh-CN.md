[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# oid-registry

Zero-allocation static X.509/PKCS/TLS/NIST/RFC OID registry with DER OID encoder and decoder (Rust oid-registry parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oid-registry` ([https://github.com/rusticata/oid-registry](https://github.com/rusticata/oid-registry))

| 元信息 | 值 |
| --- | --- |
| 包名 | `oid-registry` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add oid-registry
```

```cp
import {
    OidEntry,
    OidrDecodeStep,
    oidrEntryAt,
    oidrStrEq,
    oidEntryCount,
    lookupOid,
    lookupShortName,
    oidrEncodeBase128,
} from "oid-registry";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OidEntry` | `class` | 字段: `oid: string, short_name: string, description: string, category: string` |
| `OidrDecodeStep` | `class` | 字段: `val: u64, next_pos: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `oidrEntryAt` | `(idx: usize): OidEntry` | 调用 `oidrEntryAt` 并返回 `OidEntry`。 |
| `oidrStrEq` | `(a: string, b: string): bool` | 调用 `oidrStrEq` 并返回 `bool`。 |
| `oidEntryCount` | `(): usize` | 调用 `oidEntryCount` 并返回 `usize`。 |
| `lookupOid` | `(dotted: string): OidEntry?` | 查询或读取目标数据，返回 `OidEntry?`。 |
| `lookupShortName` | `(sn: string): OidEntry?` | 查询或读取目标数据，返回 `OidEntry?`。 |
| `oidrEncodeBase128` | `(val: u64, out: []u8, pos: usize): usize?` | 调用 `oidrEncodeBase128` 并返回 `usize?`。 |
| `oidrWriteU64Dec` | `(val: u64, out: []u8, pos: usize): usize?` | 调用 `oidrWriteU64Dec` 并返回 `usize?`。 |
| `encodeOidDer` | `(dotted: string, out: []u8): usize` | 通过 `encodeOidDer` 编码或格式化输出，返回 `usize`。 |
| `oidrDecodeBase128` | `(der: &[]u8, start: usize): OidrDecodeStep` | 调用 `oidrDecodeBase128` 并返回 `OidrDecodeStep`。 |
| `decodeOidDer` | `(der: &[]u8, out: []u8): usize` | 通过 `decodeOidDer` 解析输入数据，返回 `usize`。 |

## 最小调用示例

```cp
import { encodeOidDer } from "oid-registry";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeOidDer("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
