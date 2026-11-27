[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nibble_vec

纯 CP 实现的零堆分配 4 位半字节向量（`NibbleVec`，内联容量最高 128 个半字节 / 64 字节），专为基数树（Radix Tree）、Patricia Trie 与前缀压缩设计。

- **包名**：`nibble_vec`（`v0.1.0`）
- **上游对齐**：洁净室移植自 `crates.io/nibble_vec`（`v0.1.0`，`MIT`，`rust_nibble_vec`），采用固定栈上内联存储以彻底消除堆分配。

## 安装与导入

```bash
cap add nibble_vec
```

```typescript
import { NibbleVec, nvecMaxNibbles, nvecGetRaw } from "nibble_vec";
```

## 核心 API

### 模块函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `nvecMaxNibbles` | `(): usize` | 返回内联最大半字节容量（`128`，底层 `[64]u8`）。 |
| `nvecGetRaw` | `(data: [64]u8, idx: usize): u8` | 直接从原始 `[64]u8` 数组中读取第 `idx` 个半字节（`0..=15`）。 |

### `NibbleVec` 类

字段：`data: [64]u8`, `nibble_len: usize`。

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `new` | `(): NibbleVec` | 创建空 `NibbleVec`。 |
| `fromBytes` / `fromByteSlice` | `(bytes: &[]u8): NibbleVec` | 从字节切片构建半字节向量（高 4 位在前，最多 64 字节 / 128 半字节）。 |
| `len` / `isEmpty` / `byteLen` | `(): usize` / `bool` / `usize` | 半字节数、判空、占用字节数（`(len + 1) / 2`）。 |
| `asBytes` | `(): &[]u8` | 借用底层紧缩字节切片（`data[0..byteLen()]`）。 |
| `copyBytes` | `(dst: []u8): usize` | 将底层字节复制到 `dst`，返回复制字节数。 |
| `push` | `(nib: u8): bool` | 尾部压入 4 位半字节；满容量（`128`）时返回 `false`。 |
| `pop` | `(): u8?` | 弹出尾部半字节，空向量返回 `null`。 |
| `get` | `(idx: usize): u8?` | 读取索引 `idx` 处半字节，越界返回 `null`。 |
| `set` | `(idx: usize, nib: u8): bool` | 修改索引 `idx` 处半字节，越界返回 `false`。 |
| `clear` / `truncate` | `()` / `(new_len: usize)` | 清空向量或截断到 `new_len`。 |
| `split` | `(idx: usize): NibbleVec` | 在 `idx` 处拆分：自身保留 `[0..idx)`，返回 `[idx..len)`。 |
| `join` | `(other: NibbleVec): NibbleVec` | 返回拼接 `self` 与 `other` 的新 `NibbleVec`。 |
| `extend` | `(other: NibbleVec): bool` | 原地追加 `other`，总长超过 `128` 返回 `false`。 |
| `slice` | `(start: usize, end: usize): NibbleVec` | 返回 `[start..end)` 区间的新子向量。 |
| `commonPrefixLen` | `(other: NibbleVec): usize` | 计算与 `other` 的最长公共半字节前缀长度。 |
| `hasPrefix` / `equals` | `(other: NibbleVec): bool` | 前缀判定与逐半字节相等比较。 |

## 使用示例

### 1. 从字节构建与按索引读取

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let raw: [2]u8 = [0xAB, 0xCD];
    let nv = NibbleVec.fromBytes(raw[0..]); // 半字节: [0xA, 0xB, 0xC, 0xD]
    let n = nv.len();                       // 4
    if (let first = nv.get(0)) {
        // first == 0xA
    }
}
```

### 2. 前缀匹配与基数树拆分

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let a_bytes: [2]u8 = [0xAB, 0x12];
    let b_bytes: [2]u8 = [0xAB, 0x99];
    let a = NibbleVec.fromBytes(a_bytes[0..]);
    let b = NibbleVec.fromBytes(b_bytes[0..]);

    let lcp = a.commonPrefixLen(b); // 2 (0xA, 0xB)
    let tail = a.split(lcp);        // a = [0xA, 0xB], tail = [0x1, 0x2]
    let merged = a.join(tail);      // [0xA, 0xB, 0x1, 0x2]
}
```

### 3. 压入、弹出与切片

```typescript
import { NibbleVec } from "nibble_vec";

fn main() {
    let nv = NibbleVec.new();
    nv.push(0xF);
    nv.push(0x0);
    nv.push(0x7);
    let mid = nv.slice(1, 3); // [0x0, 0x7]
    let last = nv.pop();      // 0x7
}
```

## 与源包差异及零拷贝安全亮点

- **固定内联容量（零堆分配）**：使用 `[64]u8`（128 个半字节）栈内联存储替代 Rust `SmallVec`，零分配器依赖且内存布局确定。
- **可选类型越界保护**：`get(idx)` 与 `pop()` 返回 `u8?` 而非 panic；`push`/`set`/`extend` 返回 `bool`。
- **高位优先位布局**：与 `rust_nibble_vec` 保持一致（偶索引为高 4 位、奇索引为低 4 位），保证基数树键字典序一致。
