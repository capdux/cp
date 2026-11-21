# `cap:vmdk` — VMware VMDK 虚拟磁盘报头与 Descriptor 零分配解析库

纯 CP、零堆分配的 VMware VMDK 虚拟磁盘解析库，支持 512 字节 Little-Endian `SparseExtentHeader` 严格校验解析（含 `streamOptimized` Deflate 压缩与换行检测字节校验）、嵌入 Descriptor 零拷贝提取、文本 Descriptor（`CID`/`parentCID`/`createType`/`ddb.*`）元数据查询以及 Extent 描述行零拷贝迭代。

## 导入

```cp
import {
    VMDK_MAGIC,
    VMDK_SECTOR_SIZE,
    VMDK_HEADER_SIZE,
    VMDK_DEFAULT_GTES_PER_GT,
    FLAG_VALID_NEWLINE_DETECTOR,
    FLAG_REDUNDANT_GRAIN_TABLE,
    FLAG_ZEROED_GRAIN_GTE,
    FLAG_COMPRESSED_GRAINS,
    FLAG_MARKERS_IN_STREAM,
    COMPRESS_NONE,
    COMPRESS_DEFLATE,
    CID_NONE,
    VmdkSparseHeader,
    isVmdkSparse,
    parseVmdkSparseHeader,
    VmdkExtent,
    parseVmdkExtentLine,
    VmdkExtentIter,
    VmdkDescriptor,
    parseVmdkDescriptor,
} from "cap:vmdk";
```

## 核心 API

- **协议常量**：
  - `VMDK_MAGIC` (`0x564D444B`, `"KDMV"` LE)、`VMDK_SECTOR_SIZE` (`512`)、`VMDK_HEADER_SIZE` (`512`)、`VMDK_DEFAULT_GTES_PER_GT` (`512`)
  - Flags：`FLAG_VALID_NEWLINE_DETECTOR` (`0x0001`)、`FLAG_REDUNDANT_GRAIN_TABLE` (`0x0002`)、`FLAG_ZEROED_GRAIN_GTE` (`0x0004`)、`FLAG_COMPRESSED_GRAINS` (`0x10000`)、`FLAG_MARKERS_IN_STREAM` (`0x20000`)
  - Compression：`COMPRESS_NONE` (`0`)、`COMPRESS_DEFLATE` (`1`)
  - Descriptor：`CID_NONE` (`0xFFFFFFFF`)
- **SparseExtentHeader 解析**：
  - `isVmdkSparse(data: []const u8): bool`
  - `parseVmdkSparseHeader(data: []const u8): VmdkSparseHeader?` — 严格校验 `"KDMV"`、`version` (`1..3`)、`grain_size_sectors` (`>= 8` 且为 2 的幂)、`num_gtes_per_gt == 512`、换行检测字节（当 `FLAG_VALID_NEWLINE_DETECTOR` 置位）与 `compress_algorithm`。
  - `VmdkSparseHeader` 方法：`capacityBytes()`、`grainSizeBytes()`、`descriptorByteOffset()`、`descriptorByteSize()`、`isCompressed()`、`isStreamOptimized()`、`hasRedundantGrainTable()`、`hasValidNewlineDetector()`、`hasZeroedGrainGte()`、`embeddedDescriptor(image)`。
- **文本 Descriptor 与 Extent 解析**：
  - `parseVmdkDescriptor(text: []const u8): VmdkDescriptor?` — 解析 `version`、`CID`、`parentCID`、`createType`、`parentFileNameHint`，附 `hasParent()`、`extents()`、`findDdb(key)`。
  - `parseVmdkExtentLine(line: []const u8): VmdkExtent?` — 解析单条 Extent 行（如 `RW 20971520 SPARSE "disk-s001.vmdk"`、`RW 4096 FLAT "disk-flat.vmdk" 0`、`RDONLY 1024 ZERO`）。
  - `VmdkExtentIter` — 零拷贝遍历 Descriptor 中的所有 Extent 描述行。

## 示例

```cp
import { parseVmdkSparseHeader, parseVmdkDescriptor } from "cap:vmdk";

fn inspectVmdk(image: []const u8) {
    if (hdr := parseVmdkSparseHeader(image)) {
        * = hdr.capacityBytes();
        * = hdr.isStreamOptimized();
        if (desc_raw := hdr.embeddedDescriptor(image)) {
            if (desc := parseVmdkDescriptor(desc_raw)) {
                * = desc.findDdb("adapterType");
                let it = desc.extents();
                while (true) {
                    if (ext := it.next()) {
                        * = ext.filename;
                    } else {
                        break;
                    }
                }
            }
        }
    }
}
```
