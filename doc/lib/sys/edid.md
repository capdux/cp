# `cap:edid` — VESA EDID 1.3 / 1.4 显示器识别数据块零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

纯 CP、零堆分配的 VESA E-EDID 1.3 / 1.4 128 字节基础块解析库，支持 5-bit 压缩 PNP 厂商 ID 编解码、Detailed Timing Descriptor (DTD) 刷新率计算、Monitor Name / Serial 描述符零拷贝提取、Standard Timing 宽高比解析以及 128 字节校验和验证。

## 导入

```cp
import {
    EDID_BLOCK_SIZE,
    EDID_HEADER_MAGIC,
    DESC_SERIAL,
    DESC_UNSPECIFIED_TEXT,
    DESC_RANGE_LIMITS,
    DESC_MONITOR_NAME,
    DESC_DUMMY,
    CEA_EXT_TAG,
    edidChecksum,
    verifyEdidChecksum,
    decodePnpId,
    encodePnpId,
    EdidDetailedTiming,
    EdidDescriptor,
    EdidStandardTiming,
    EdidInfo,
    isEdid,
    parseEdid,
} from "cap:sys";
```

## 核心 API

- **校验和与识别**：`edidChecksum(block128): u8`、`verifyEdidChecksum(block128): bool`、`isEdid(data): bool`、`parseEdid(data): EdidInfo?`。
- **PNP ID 编解码**：`decodePnpId(raw_be: u16): [3]u8`、`encodePnpId(id: [3]u8): u16?`（如 `"DEL"` / `"SAM"` / `"APP"`）。
- **EDID 信息与描述符访问**：`EdidInfo` 提供 `gamma(): f64`、`preferredTiming(): EdidDetailedTiming?`（附 `refreshRateHz(): f64`）、`monitorName(): ([]const u8)?`、`serialText(): ([]const u8)?`、`descriptorAt(idx: 0..3): EdidDescriptor?`、`standardTimingAt(idx: 0..7): EdidStandardTiming?`。

## 示例

```cp
import { parseEdid } from "cap:sys";

fn example(edid_bytes: []const u8) {
    if (info := parseEdid(edid_bytes)) {
        if (pt := info.preferredTiming()) {
            * = pt.h_active;
            * = pt.refreshRateHz();
        }
        * = info.monitorName();
    }
}
```
