[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# nibble_vec

零堆分配的 4-bit 半字节向量（`NibbleVec`，内联容量 128 个 nibble / 64 字节），支持 `push`/`pop`、`split`/`join`/`extend`/`slice` 以及前缀匹配（`commonPrefixLen`、`hasPrefix`），专为基数树（Radix Tree）与 Trie 前缀压缩设计。移植自 Rust `nibble_vec`。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `nibble_vec` (`michaelsproul/rust_nibble_vec`) |
| **CP 包名** | `nibble_vec` (`v0.1.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `nibble_vec` 不变，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: nibble_vec)
use nibble_vec::Nibblet;
let mut nv = Nibblet::from_byte_vec(vec![0xAB, 0xCD]);
assert_eq!(nv.get(0), 0xA);
```

```typescript
// CP（保持原包名 "nibble_vec"）
import { NibbleVec } from "nibble_vec";
let raw: [2]u8 = [0xAB, 0xCD];
let nv = NibbleVec.fromBytes(raw[0..2]); // nibbles: [0xA, 0xB, 0xC, 0xD]
```

## 安装

```bash
cap add nibble_vec
```

## 快速上手与实战示例

### 1. 构造、索引与原地修改

```typescript
import { NibbleVec, nvecMaxNibbles } from "nibble_vec";

fn basicOps(): void {
    let cap = nvecMaxNibbles(); // 128 个半字节（64 字节）

    let mut nv = NibbleVec.new();
    let ok1 = nv.push(0xA);
    let ok2 = nv.push(0xB);
    let ok3 = nv.push(0xC);

    let len = nv.len();       // 3
    let bytes = nv.byteLen(); // 2
    let first = nv.get(0);    // 0xA?
    let updated = nv.set(1, 0xF);
    let popped = nv.pop();    // 0xC?

    // 从字节切片构造（高 4 位在前，低 4 位在后）
    let src: [2]u8 = [0x12, 0x34];
    let mut from_b = NibbleVec.fromBytes(src[0..2]); // 别名：fromByteSlice
    let view = from_b.asBytes();

    let mut dst: [8]u8 = undefined;
    let copied = from_b.copyBytes(dst[0..8]); // 2

    nv.truncate(1);
    nv.clear();
}
```

### 2. 基数树操作：`split`、`join`、`extend`、`slice` 与前缀匹配

```typescript
import { NibbleVec } from "nibble_vec";

fn radixOps(): void {
    let b1: [2]u8 = [0xAB, 0xCD];
    let b2: [2]u8 = [0xAB, 0xEF];

    let mut a = NibbleVec.fromBytes(b1[0..2]); // [A, B, C, D]
    let b = NibbleVec.fromBytes(b2[0..2]);     // [A, B, E, F]

    let lcp = a.commonPrefixLen(b);            // 2 ([A, B])
    let prefix = a.slice(0, lcp);              // [A, B]
    let has_pre = b.hasPrefix(prefix);         // true

    // 在索引 2 处原地拆分：a 保留 [A, B]，返回尾部 [C, D]
    let tail = a.split(2);

    // 拼接与原地扩展
    let joined = a.join(tail);                 // [A, B, C, D]
    let eq = joined.equals(NibbleVec.fromBytes(b1[0..2])); // true

    let mut extended = a;
    let ok = extended.extend(tail);
}
```

## API 速览

### 常量与辅助函数
- `NIBBLE_MAX_BYTES: usize`（`64`）、`nvecMaxNibbles(): usize`（`128`）、`nvecGetRaw(data: [64]u8, idx: usize): u8`

### `NibbleVec`
- 构造：`new()`、`fromBytes(&[]u8)`、`fromByteSlice(&[]u8)`
- 查询：`len()`、`isEmpty()`、`byteLen()`、`asBytes()`、`copyBytes([]u8)`、`get(idx): u8?`
- 修改：`push(nib): bool`、`pop(): u8?`、`set(idx, nib): bool`、`clear()`、`truncate(new_len)`、`split(idx): NibbleVec`、`extend(other): bool`
- 组合与前缀：`join(other)`、`slice(start, end)`、`commonPrefixLen(other)`、`hasPrefix(prefix)`、`equals(other)`
