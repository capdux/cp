# `cap:qcow2` — QEMU QCOW2 v2/v3 虚拟磁盘镜像零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配的 QEMU QCOW2 v2/v3 虚拟磁盘镜像解析库，支持 72B/104B 报头解析、Feature 标志位判定、Backing File 零拷贝提取、8B 对齐 Header Extensions 迭代以及 64-bit L1/L2 簇表项位域解码。

## 导入

```cp
import {
    QCOW2_MAGIC,
    QCOW2_V2_HEADER_SIZE,
    QCOW2_V3_MIN_HEADER_SIZE,
    QCOW2_MIN_CLUSTER_BITS,
    QCOW2_MAX_CLUSTER_BITS,
    CRYPT_NONE,
    CRYPT_AES,
    CRYPT_LUKS,
    INCOMPAT_DIRTY,
    INCOMPAT_CORRUPT,
    INCOMPAT_EXTERNAL_DATA,
    INCOMPAT_COMPRESSION_TYPE,
    INCOMPAT_EXTENDED_L2,
    EXT_END,
    EXT_BACKING_FORMAT,
    EXT_FEATURE_NAME_TABLE,
    EXT_BITMAPS,
    EXT_FULL_DISK_ENCRYPTION,
    Qcow2Header,
    isQcow2,
    parseQcow2Header,
    Qcow2Ext,
    Qcow2ExtIter,
    iterQcow2Exts,
    Qcow2L1Entry,
    decodeL1Entry,
    Qcow2L2Entry,
    decodeL2Entry,
} from "cap:sys";
```

## 核心 API

- **报头解析**：`isQcow2(data): bool`、`parseQcow2Header(data): Qcow2Header?`（附 `clusterSize()`, `refcountBits()`, `hasBackingFile()`, `isDirty()`, `isCorrupt()`, `hasExtendedL2()`, `backingFile(image)`）。
- **Header Extensions**：`iterQcow2Exts(header, data): Qcow2ExtIter`（8B 对齐步进，遇 `EXT_END (0)` 停止，附 `sawEnd()` / `isMalformed()`）。
- **L1 / L2 簇表项**：`decodeL1Entry(raw): Qcow2L1Entry`（`isCopied()`, `l2Offset()`）、`decodeL2Entry(raw): Qcow2L2Entry`（`isCopied()`, `isCompressed()`, `isZeroCluster()`, `hostOffset()`）。

## 示例

```cp
import { parseQcow2Header, iterQcow2Exts, EXT_BACKING_FORMAT } from "cap:sys";

fn example(img: []const u8) {
    if (hdr := parseQcow2Header(img)) {
        * = hdr.clusterSize();
        let it = iterQcow2Exts(hdr, img);
        while (true) {
            if (ext := it.next()) {
                if (ext.ext_type == EXT_BACKING_FORMAT) {
                    * = ext.data;
                }
            } else {
                break;
            }
        }
    }
}
```
