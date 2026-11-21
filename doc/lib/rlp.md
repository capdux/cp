# `cap:rlp` — 纯 CP Ethereum RLP 确定性二进制编解码库

## 概述

`cap:rlp` 提供纯 CP、零外部依赖、零堆分配的 Ethereum Recursive Length Prefix（Yellow Paper Appendix B）确定性编解码器：
- **严格规范性解码**：拒绝一切非规范编码（单字节 `< 0x80` 使用 `0x81` 前缀、长长度前导 `0x00`、`< 56` 使用长前缀、整数前导 `0x00`、`> 8` 字节 `u64` 与截断越界）；
- **零堆分配编码**：`RlpWriter` 直接向用户提供的 `[]u8` 缓冲区原子写入字节串、最小大端 `u64` 与列表头，并提供 `encodedBytesLen` / `encodedU64Len` / `encodedListHeaderLen` 预计算。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `RlpKind` | RLP 项类型：`bytes`、`list` |
| `RlpItem` / `decodeRlpItem(data)` | 严格规范性解码单个 RLP 项，返回 `kind`、零拷贝 `payload` 与 `total_bytes` |
| `RlpListIter` | 流式迭代列表子项：`init(list_payload)`、`next(*_): RlpItem?`、`isDone()` |
| `decodeRlpU64(data)` | 严格解码规范无符号 64 位整数（`0x80` -> `0`） |
| `RlpWriter` | 零分配编码器：`init(buf)`、`writeBytes`、`writeU64`、`writeListHeader`、`written()` |
| `encodedBytesLen`, `encodedU64Len`, `encodedListHeaderLen` | 编码长度预计算辅助函数 |

## 示例

```cp
import { RlpWriter, decodeRlpItem, RlpListIter, encodedBytesLen } from "cap:rlp";

fn encodePair(out: []u8): []const u8 {
    let w = RlpWriter.init(out);
    const body_len = encodedBytesLen("cat") + encodedBytesLen("dog");
    * = w.writeListHeader(body_len);
    * = w.writeBytes("cat");
    * = w.writeBytes("dog");
    return w.written();
}
```
