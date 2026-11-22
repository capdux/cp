# `cap:ktx` — 纯 CP 零分配 Khronos KTX 1.1 & KTX2 GPU 纹理容器解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

## 概述

`cap:ktx` 提供纯 CP、零外部依赖、零堆分配的 Khronos KTX 1.1（64 字节大小端自适应头）与 KTX 2.0（80 字节头与 24 字节 Mipmap Level Index）解析器，适用于 WebGPU / Vulkan / OpenGL 纹理资产加载管线。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `KtxVersion` | 版本枚举：`ktx1`, `ktx2`, `unknown` |
| `isKtx1(data)` / `isKtx2(data)` / `detectKtxVersion(data)` | 12 字节官方魔数检测（`«KTX 11»\r\n\x1a\n` / `«KTX 20»\r\n\x1a\n`） |
| `Ktx1Header` | KTX 1.1 64 字节头（`is_big_endian`, `gl_type`, `gl_type_size`, `gl_format`, `gl_internal_format`, `gl_base_internal_format`, `pixel_width`/`height`/`depth`, `array_elements`, `faces`, `mipmap_levels`, `bytes_of_key_value_data`；方法 `dataOffset(): usize`） |
| `parseKtx1Header(data)` | 解析 KTX 1.1 头（自动识别 `0x04030201` 大小端并校验 KVD 边界） |
| `Ktx2Header` | KTX 2.0 80 字节头（`vk_format`, `type_size`, `pixel_width`/`height`/`depth`, `layer_count`, `face_count`, `level_count`, `supercompression_scheme`, DFD/KVD/SGD 偏移与长度；方法 `effectiveLevels(): u32`） |
| `parseKtx2Header(data)` | 解析 KTX2 头并校验 DFD/KVD/SGD 索引不越界 |
| `Ktx2Level` | 24 字节 Level Index 条目（`byte_offset`, `byte_length`, `uncompressed_byte_length`；方法 `payload(data): ([]const u8)?`） |
| `readKtx2Level(data, level_idx)` | 零拷贝读取第 `level_idx` 级 Mipmap Level Index 并校验表项与负载不越界 |

## 示例

```cp
import { isKtx2, parseKtx2Header, readKtx2Level } from "cap:gfx";

fn getBaseMipPayload(ktx2: []const u8): ([]const u8)? {
    if (!isKtx2(ktx2)) return null;
    * = parseKtx2Header(ktx2) ?? return null;
    const lvl0 = readKtx2Level(ktx2, 0) ?? return null;
    return lvl0.payload(ktx2);
}
```
