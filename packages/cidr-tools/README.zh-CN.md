[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cidr-tools

Zero-allocation IPv4 CIDR parser, formatter, overlap/containment checker, merge, and exclude engine (npm cidr-tools / ip-cidr parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cidr-tools` ([https://github.com/silverwind/cidr-tools](https://github.com/silverwind/cidr-tools))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cidr-tools` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cidr-tools
```

```cp
import {
    Cidr4,
    Range64,
    invalidCidr,
    pow2_64,
    prefixMask,
    parseIpv4,
    parseCidr4,
    appendU8Dec,
} from "cidr-tools";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Cidr4` | `class` | 字段: `network: u32, prefix: u8, valid: bool` |
| `Range64` | `class` | 字段: `lo: u64, hi: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidCidr` | `(): Cidr4` | 调用 `invalidCidr` 并返回 `Cidr4`。 |
| `pow2_64` | `(n: u8): u64` | 调用 `pow2_64` 并返回 `u64`。 |
| `prefixMask` | `(prefix: u8): u32` | 调用 `prefixMask` 并返回 `u32`。 |
| `parseIpv4` | `(s: &[]u8): i64` | 通过 `parseIpv4` 解析输入数据，返回 `i64`。 |
| `parseCidr4` | `(s: &[]u8): Cidr4` | 通过 `parseCidr4` 解析输入数据，返回 `Cidr4`。 |
| `appendU8Dec` | `(dst: []u8, pos: usize, v_in: u8): usize` | 调用 `appendU8Dec` 并返回 `usize`。 |
| `formatCidr4` | `(c: Cidr4, dst: []u8): usize` | 通过 `formatCidr4` 编码或格式化输出，返回 `usize`。 |
| `cidrHostCount` | `(c: Cidr4): u64` | 调用 `cidrHostCount` 并返回 `u64`。 |
| `cidrStart` | `(c: Cidr4): u32` | 调用 `cidrStart` 并返回 `u32`。 |
| `cidrEnd` | `(c: Cidr4): u32` | 调用 `cidrEnd` 并返回 `u32`。 |
| `containsIp` | `(c: Cidr4, ip: u32): bool` | 调用 `containsIp` 并返回 `bool`。 |
| `containsCidr` | `(outer: Cidr4, inner: Cidr4): bool` | 调用 `containsCidr` 并返回 `bool`。 |
| `overlaps` | `(a: Cidr4, b: Cidr4): bool` | 调用 `overlaps` 并返回 `bool`。 |
| `emitRangeCidrs` | `(lo_in: u64, hi: u64, out: []Cidr4, pos_in: usize): usize` | 调用 `emitRangeCidrs` 并返回 `usize`。 |
| `mergeCidrs` | `(inputs: &[]Cidr4, out: []Cidr4): usize` | 调用 `mergeCidrs` 并返回 `usize`。 |
| `excludeCidr` | `(base: Cidr4, remove: Cidr4, out: []Cidr4): usize` | 调用 `excludeCidr` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseIpv4 } from "cidr-tools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIpv4(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
