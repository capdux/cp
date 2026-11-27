[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bl

面向 CP 语言的零堆分配分段缓冲区列表（`BufferList`），支持跨分块流式字节读取、大端/小端整数解码、`indexOf` 搜索、`copyRange` 与 `consume`。

对齐 npm 源包 [`bl`](https://github.com/rvagg/bl) `6.1.0`（MIT）。

## 安装与导入

```bash
cap add bl
```

```cp
import {
    BL_MAX_CHUNKS,
    BufferList,
    newBufferList,
} from "bl";
```

## 核心 API

### 常量与构造器

| 符号 | 签名 / 值 | 说明 |
| --- | --- | --- |
| `BL_MAX_CHUNKS` | `usize = 64` | 单个 `BufferList` 最多同时持有的借用分块数。 |
| `newBufferList` | `(): BufferList` | 在栈上创建一个空的 `BufferList`。 |

### `BufferList` 类

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 字段 | `chunks: [64]&[]u8`, `chunk_count: usize`, `total_len: usize` | 固定容量的分块只读切片数组、有效块数与总字节长度。 |
| `len` | `(): usize` | 返回全部已追加分块的可读总字节数。 |
| `isEmpty` | `(): bool` | 当 `total_len == 0` 时返回 `true`。 |
| `append` | `(*self, chunk: &[]u8): bool` | 零拷贝追加非空只读分块；达到 64 块上限时返回 `false`。 |
| `get` | `(index: usize): u8?` | 跨分块按逻辑索引读取单字节，越界返回 `null`。 |
| `copyRange` | `(start: usize, end: usize, dst: []u8): usize` | 将逻辑区间 `[start, end)` 跨分块拷贝到 `dst`，返回实际拷贝字节数。 |
| `consume` | `(*self, n: usize)` | 原地丢弃前 `n` 字节（自动切分跨界首块并回收已耗尽分块）。 |
| `readU16BE` / `readU16LE` | `(offset: usize): u16?` | 跨分块无缝读取大端/小端 `u16`，越界返回 `null`。 |
| `readU32BE` / `readU32LE` | `(offset: usize): u32?` | 跨分块无缝读取大端/小端 `u32`，越界返回 `null`。 |
| `indexOf` | `(needle: &[]u8, from_offset: usize): usize?` | 从 `from_offset` 起跨分块查找子序列 `needle` 首次出现的逻辑偏移，未找到返回 `null`。 |

## 使用示例

### 1. 跨分块二进制协议帧解码

```cp
import { print } from "cap:io";
import { newBufferList } from "bl";

pub fn main() {
    let bl = newBufferList();
    let c1: [2]u8 = [0x01, 0x02];
    let c2: [4]u8 = [0x03, 0x04, 0xAA, 0xBB];
    let _ = bl.append(c1[0..2]);
    let _ = bl.append(c2[0..4]);

    let magic = bl.readU32BE(0);
    if (magic != null) {
        print("magic=0x{:x}, len={}\n", magic!, bl.len()); // 0x01020304, len=6
    }
    bl.consume(4);
    print("remaining={}\n", bl.len()); // 2
}
```

### 2. 流式分隔符搜索（`indexOf`）与区间提取

```cp
import { print } from "cap:io";
import { newBufferList } from "bl";

pub fn main() {
    let bl = newBufferList();
    let _ = bl.append("GET /index");
    let _ = bl.append(".html HTTP/1.1\r\n");

    let pos = bl.indexOf("\r\n", 0);
    if (pos != null) {
        let line: [64]u8 = undefined;
        let n = bl.copyRange(0, pos!, line[0..64]);
        print("request line: {}\n", line[0..n]);
        bl.consume(pos! + 2);
    }
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：`BufferList` 在栈上固定存储最多 `BL_MAX_CHUNKS = 64` 个 `&[]u8` 分块引用，无需 `Buffer.concat` 拼接拷贝，也无链表节点堆分配。
- **透明跨块读写**：`readU16BE/LE`、`readU32BE/LE`、`copyRange` 与 `indexOf` 自动跨越任意分块边界。
- **可选类型越界保护**：所有越界读取均返回 `null`（`u8?` / `u16?` / `u32?` / `usize?`），杜绝运行时 `RangeError` 崩溃。
