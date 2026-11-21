# `cap:spv` — 纯 CP 零分配 Khronos SPIR-V 二进制着色器解析库

## 概述

`cap:spv` 提供纯 CP、零外部依赖、零堆分配的 Khronos SPIR-V 二进制着色器 20 字节文件头解析（自动兼容 LE/BE）、指令流迭代器、`OpEntryPoint` 入口提取与单遍模块统计摘要，与 `cap:wgsl`、`cap:glsl` 配套用于 GPU 着色器管线。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| 常量 | `SPV_MAGIC = 0x07230203`、`SPV_HEADER_BYTES = 20` 及常用 Opcode（`OP_NOP`, `OP_SOURCE`, `OP_NAME`, `OP_EXT_INST_IMPORT`, `OP_MEMORY_MODEL`, `OP_ENTRY_POINT`, `OP_EXECUTION_MODE`, `OP_CAPABILITY`, `OP_TYPE_VOID`, `OP_FUNCTION`, `OP_FUNCTION_END`） |
| `isSpv(data)` | 检测小端/大端魔数、长度 `>= 20` 且 4 字节对齐 |
| `SpvHeader` & `parseSpvHeader(data)` | 解析 `is_big_endian`, `version_major`, `version_minor`, `generator_magic`, `bound`, `schema`, `word_count` |
| `SpvInst` & `SpvInstIter` | 零分配逐条迭代指令（校验 `word_count >= 1` 且不越界）；方法 `operandCount(): u16`、`operandU32(idx: u16): u32?` |
| `SpvExecutionModel` & `SpvEntryPoint` & `parseSpvEntryPoint(inst, data, is_be)` | 解析 `OpEntryPoint` 的执行模型、函数 ID 与 `\0` 结尾 UTF-8 入口名 |
| `SpvStats` & `summarizeSpv(data)` | 单遍统计指令总数、Capability、EntryPoint 与 Function 数量 |

## 示例

```cp
import { isSpv, parseSpvHeader, summarizeSpv } from "cap:spv";

fn inspectShader(spv_bytes: []const u8): u32? {
    if (!isSpv(spv_bytes)) return null;
    * = parseSpvHeader(spv_bytes) ?? return null;
    const stats = summarizeSpv(spv_bytes) ?? return null;
    return stats.entry_point_count;
}
```
