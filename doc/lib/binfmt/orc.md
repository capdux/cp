# `cap:orc` — 纯 CP Apache ORC 列式存储容器零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 概述

`cap:orc` 提供纯 CP、零外部依赖、零堆分配的 Apache ORC 列式存储容器解析器，内置零分配 Protobuf Varint/Wire-Type 解码辅助，支持 ORC 尾部 PostScript 元数据解析（版本号、压缩算法、Footer/Metadata 长度与 field 8000 `"ORC"` 魔数校验）、严格防下溢的文件分区布局切片提取以及 3 字节小端压缩/原始数据块流式迭代。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ORC_MAGIC` | 3 字节魔数常量 `"ORC"` |
| `OrcCompression` | 压缩算法枚举：`none=0`, `zlib=1`, `snappy=2`, `lzo=3`, `lz4=4`, `zstd=5`, `unknown=255` |
| `OrcPostScript` | 尾部 PostScript 元数据：`footer_length`, `compression`, `compression_block_size`, `version_major`, `version_minor`, `metadata_length`, `writer_version`, `magic`, `postscript_len` |
| `OrcLayout` | 分区零拷贝视图：`postscript`, `postscript_offset`, `footer_offset`, `footer_slice`, `metadata_offset`, `metadata_slice`, `stripes_slice` |
| `OrcCompressionBlock` | 3B 小端压缩帧视图：`is_original`, `length`, `payload` |
| `readVarint(buf, start)` / `skipField(buf, start, wire_type)` | 零分配 Protobuf 底层解码辅助 |
| `isOrc(data)` / `parseOrcPostScript(data)` / `parseOrcLayout(data)` | ORC 识别、PostScript 解析与分区切片校验 |
| `OrcBlockIter` | 3 字节头压缩/原始块流式迭代器：`init(stream)`、`next(*_): OrcCompressionBlock?` |

## 示例

```cp
import { parseOrcLayout, OrcCompression } from "cap:binfmt";

fn inspectOrc(file_bytes: []const u8): bool {
    if (layout := parseOrcLayout(file_bytes)) {
        return layout.postscript.compression != OrcCompression.unknown && layout.footer_slice.len > 0;
    }
    return false;
}
```
