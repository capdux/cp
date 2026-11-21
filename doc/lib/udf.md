# `cap:udf` — ECMA-167 / ISO 13346 UDF 通用光盘格式零分配解析库

纯 CP、零堆分配的 UDF (Universal Disk Format, ECMA-167 / OSTA UDF) 解析库，支持卷识别序列 (VRS: `BEA01`/`NSR02`/`NSR03`/`TEA01`)、16B Descriptor Tag 校验和与 CRC-ITU-T (`0x1021`) 校验、AVDP (Tag 2) 主/备 VDS Extent 解析、PVD (Tag 1) 解析以及 OSTA CS0 (8-bit / UTF-16BE) 压缩字符串解码。

## 导入

```cp
import {
    UDF_SECTOR_SIZE,
    UDF_VRS_OFFSET,
    UDF_AVDP_SECTOR,
    UDF_TAG_SIZE,
    VRS_BEA01,
    VRS_NSR02,
    VRS_NSR03,
    VRS_TEA01,
    TAG_PVD,
    TAG_AVDP,
    TAG_VDP,
    TAG_IUVD,
    TAG_PD,
    TAG_LVD,
    TAG_USD,
    TAG_TD,
    TAG_LVID,
    TAG_FSD,
    TAG_FID,
    TAG_FE,
    TAG_EFE,
    udfTagChecksum,
    udfCrc16,
    UdfExtentAd,
    UdfTag,
    parseUdfTag,
    UdfVrsEntry,
    parseUdfVrsEntry,
    UdfAvdp,
    parseUdfAvdp,
    decodeOstaCs0Ascii,
    UdfPvd,
    parseUdfPvd,
} from "cap:udf";
```

## 核心 API

- **校验和**：
  - `udfTagChecksum(tag16: []const u8): u8` — 跳过字节 4 对前 16B 求和 mod 256。
  - `udfCrc16(data: []const u8): u16` — ECMA-167 CRC-ITU-T（多项式 `0x1021`、非反射、初值 `0x0000`、无最终异或）。
- **VRS 与 Descriptor 解析**：
  - `parseUdfVrsEntry(sector2k: []const u8): UdfVrsEntry?`（附 `isBea01()`, `isNsr02()`, `isNsr03()`, `isTea01()`）。
  - `parseUdfTag(data: []const u8): UdfTag?` — 严格校验 16B Tag Checksum，当 `16 + desc_crc_len <= data.len` 时同时校验 `udfCrc16`。
  - `parseUdfAvdp(sector: []const u8): UdfAvdp?` — 校验 `tag_id == TAG_AVDP (2)`，提取 `main_vds` 与 `reserve_vds`。
  - `parseUdfPvd(sector: []const u8): UdfPvd?` — 校验 `tag_id == TAG_PVD (1)`，附 `decodeVolIdAscii(out)`。
- **OSTA CS0 解码**：
  - `decodeOstaCs0Ascii(dstring: []const u8, out: []u8): usize?` — 支持 `comp_id == 8`（8-bit）与 `comp_id == 16`（大端 UTF-16BE），自动依据末字节长度解码。

## 示例

```cp
import { parseUdfAvdp, parseUdfPvd } from "cap:udf";

fn inspectUdf(avdp_sec: []const u8, pvd_sec: []const u8) {
    if (avdp := parseUdfAvdp(avdp_sec)) {
        * = avdp.main_vds.location;
    }
    if (pvd := parseUdfPvd(pvd_sec)) {
        let vol_name: [32]u8 = [0; 32];
        * = pvd.decodeVolIdAscii(vol_name[0..]);
    }
}
```
