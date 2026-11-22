# `cap:arrow` — 纯 CP Apache Arrow IPC Stream / File 容器帧与 Footer 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 概述

`cap:arrow` 提供纯 CP、零外部依赖、零堆分配的 Apache Arrow IPC File 与 IPC Stream 二进制帧解析器、尾部 Footer 提取器与安全 FlatBuffers 根表/vtable 低层读取器，支持 0.15+ `0xFFFFFFFF` 续帧与旧版格式、EOS 终止标记、8 字节对齐跳过及 `Message` 元数据版本/类型提取。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ARROW_MAGIC` / `ARROW_CONTINUATION` | 常量：`"ARROW1"`、`0xFFFFFFFF` |
| `ArrowMetadataVersion` | 元数据版本枚举：`v1`, `v2`, `v3`, `v4`, `v5`, `unknown` |
| `ArrowHeaderType` | `MessageHeader` 类型枚举：`none`, `schema` (1), `dictionary_batch` (2), `record_batch` (3), `tensor` (4), `sparse_tensor` (5), `unknown` |
| `ArrowMessageMeta` / `parseArrowMessageMeta(metadata)` | 从 FlatBuffers `Message` 根表提取 `version`、`header_type` 与 `body_length` |
| `ArrowMessageHeader` / `ArrowMessageIter` | 流式消息迭代器：`init(data)`, `next(*_): ArrowMessageHeader?`, `skipBody(*_, body_length): bool`；兼容 `0xFFFFFFFF` 续帧与旧版格式、支持 `metadata_len == 0` EOS 与自动 8 字节对齐 |
| `ArrowFooterInfo` / `parseArrowFileFooter(data)` | 校验首尾 `"ARROW1"` 及前 4B LE `footer_len`，返回零拷贝 `footer_flatbuf` 与 `stream_slice` |
| `isArrowFile(data)` / `isArrowStream(data)` | 快速识别 Arrow File 与 Arrow IPC Stream |
| `readFbU32` / `readFbRootTable` / `readFbVtableOffset` | 安全边界检查的 FlatBuffers 低层读取辅助 |

## 示例

```cp
import { isArrowFile, parseArrowFileFooter, ArrowMessageIter } from "cap:binfmt";

fn countArrowMessages(file_bytes: []const u8): usize {
    if (!isArrowFile(file_bytes)) return 0;
    const footer = parseArrowFileFooter(file_bytes) ?? return 0;
    let it = ArrowMessageIter.init(footer.stream_slice);
    let n: usize = 0;
    while (msg := it.next()) {
        if (msg.is_eos) break;
        n += 1;
    }
    return n;
}
```
