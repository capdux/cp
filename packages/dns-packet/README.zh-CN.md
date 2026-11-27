[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dns-packet

Zero-heap RFC 1035 DNS binary packet encoder/decoder with 12-byte header, domain label encoding, 0xC0 compression pointer decoding, and query builder.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dns-packet` ([https://github.com/mafintosh/dns-packet](https://github.com/mafintosh/dns-packet))

| 元信息 | 值 |
| --- | --- |
| 包名 | `dns-packet` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add dns-packet
```

```cp
import {
    DnsHeader,
    NameDecodeResult,
    putBeU16,
    getBeU16,
    encodeHeader,
    decodeHeader,
    encodeName,
    decodeName,
} from "dns-packet";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DnsHeader` | `class` | 字段: `id: u16, flags: u16, qdcount: u16, ancount: u16, nscount: u16, arcount: u16` |
| `NameDecodeResult` | `class` | 字段: `name_len: usize, next_offset: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `putBeU16` | `(dst: []u8, offset: usize, v: u16)` | 写入或更新状态数据。 |
| `getBeU16` | `(src: &[]u8, offset: usize): u16` | 查询或读取目标数据，返回 `u16`。 |
| `encodeHeader` | `(h: DnsHeader, dst: []u8): usize` | 通过 `encodeHeader` 编码或格式化输出，返回 `usize`。 |
| `decodeHeader` | `(src: &[]u8): DnsHeader` | 通过 `decodeHeader` 解析输入数据，返回 `DnsHeader`。 |
| `encodeName` | `(domain: &[]u8, dst: []u8): i64` | 通过 `encodeName` 编码或格式化输出，返回 `i64`。 |
| `decodeName` | `(packet: &[]u8, offset: usize, dst: []u8): NameDecodeResult` | 通过 `decodeName` 解析输入数据，返回 `NameDecodeResult`。 |
| `encodeQuery` | `(id: u16, domain: &[]u8, qtype: u16, rd: bool, dst: []u8): i64` | 通过 `encodeQuery` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { getBeU16 } from "dns-packet";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getBeU16(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
