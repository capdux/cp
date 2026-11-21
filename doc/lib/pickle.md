# `cap:pickle` — 纯 CP Python Pickle Protocol 2–5 二进制指令流解析与静态审计库

## 概述

`cap:pickle` 提供纯 CP、零外部依赖、零堆分配的 Python Pickle Protocol 2–5 二进制操作码流解析器与安全静态审计器，专为 ML 模型权重包（`.pkl`/`.pt`）上线前的静态指令检查与安全闸门设计。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `OP_PROTO` .. `OP_FROZENSET` | Python Pickle Protocol 2–5 全部核心 Opcode 常量 |
| `detectPickleProto(data): u8?` / `isPickle(data): bool` | 校验 `OP_PROTO` (`0x80`) 与协议版本 `2..5` |
| `isUnsafePickleOp(op: u8): bool` | 判断指令是否属于可执行构造/扩展操作码（`GLOBAL`/`STACK_GLOBAL`/`REDUCE`/`BUILD`/`NEWOBJ`/`NEWOBJ_EX`/`INST`/`OBJ`/`PERSID`/`BINPERSID`/`EXT1..4`） |
| `PickleOp` | 单条解码后的指令：`opcode`, `offset`, `byte_len`, `int_val`, `uint_val`, `float_val`, `text`, `text2` |
| `PickleIter` | 零分配逐条操作码迭代器：`init(data)`, `next(*_): PickleOp?`（遇 `OP_STOP` 返回该指令后停止） |
| `PickleAudit` / `auditPickle(data)` | 单遍静态审计汇总：`proto`, `op_count`, `global_count`, `reduce_count`, `frame_count`, `has_stop`, `has_unsafe_ops`, `valid` |

## 示例

```cp
import { isPickle, auditPickle } from "cap:pickle";

fn checkModelWeights(bytes: []const u8): bool {
    if (!isPickle(bytes)) return false;
    const audit = auditPickle(bytes);
    return audit.valid && !audit.has_unsafe_ops;
}
```
