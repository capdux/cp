[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cuckoofilter

Zero-allocation CoNEXT'14 Cuckoo Filter (256x4 16-bit fingerprint slots) with O(1) insert, contains, and safe delete for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cuckoofilter` ([https://github.com/seiflotfy/rust-cuckoofilter](https://github.com/seiflotfy/rust-cuckoofilter))（原库名 `cuckoofilter` → CP 包名 `cuckoofilter`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `cuckoofilter` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cuckoofilter
```

```cp
import {
    CuckooFilter,
    mulWrap64,
    avalanche64,
    hashBytes,
    fingerprintFromHash,
    altBucket,
} from "cuckoofilter";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CuckooFilter` | `class` | 字段: `buckets: [1024]u16, item_count: usize, rng_state: u64` · 方法: `init, len, capacity, isEmpty, loadFactor, clear, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `avalanche64` | `(x: u64): u64` | 调用 `avalanche64` 并返回 `u64`。 |
| `hashBytes` | `(data: &[]u8): u64` | 执行 `hashBytes` 条件判定，返回 `bool`。 |
| `fingerprintFromHash` | `(h: u64): u16` | 调用 `fingerprintFromHash` 并返回 `u16`。 |
| `altBucket` | `(bucket: usize, fp: u16): usize` | 调用 `altBucket` 并返回 `usize`。 |

## 最小调用示例

```cp
import { hashBytes } from "cuckoofilter";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
