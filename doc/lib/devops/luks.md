# `cap:luks` — Linux LUKS1 & LUKS2 磁盘加密报头零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

纯 CP、零堆分配的 Linux Unified Key Setup (LUKS1 / LUKS2) 磁盘加密报头解析库，支持 LUKS1 592B 报头与 8 个密钥槽解析、LUKS2 4096B 主/备报头（`"LUKS\xBA\xBE"` / `"SKUL\xBA\xBE"`）解析、`seqid` 主备择优以及 `[4096..hdr_size]` JSON 元数据区零拷贝 NUL 剥离提取。

## 导入

```cp
import {
    LUKS_MAGIC,
    LUKS2_MAGIC_SECONDARY,
    LUKS_V1,
    LUKS_V2,
    LUKS1_HEADER_SIZE,
    LUKS1_NUM_KEYSLOTS,
    LUKS1_KEYSLOT_SIZE,
    LUKS1_KEY_ENABLED,
    LUKS1_KEY_DISABLED,
    LUKS2_BIN_HEADER_SIZE,
    LUKS2_MIN_HDR_SIZE,
    peekLuksVersion,
    isLuks,
    Luks1Keyslot,
    Luks1Header,
    parseLuks1Header,
    Luks2Header,
    parseLuks2Header,
    selectActiveLuks2,
} from "cap:devops";
```

## 核心 API

- **版本嗅探**：
  - `peekLuksVersion(data: []const u8): u16?` — 识别主魔数 `"LUKS\xBA\xBE"` (v1/v2) 与备魔数 `"SKUL\xBA\xBE"` (v2)。
  - `isLuks(data: []const u8): bool`
- **LUKS1 报头与密钥槽**：
  - `parseLuks1Header(data: []const u8): Luks1Header?` — 严格校验 6B 魔数、`version == 1`、`len >= 592`、`key_bytes > 0`。
  - `Luks1Header` 方法：`cipherNameSlice()`、`cipherModeSlice()`、`hashSpecSlice()`、`uuidSlice()`、`activeKeyslotCount()`。
  - `Luks1Keyslot` 方法：`isEnabled()`（`0x00AC71F3`）、`isDisabled()`（`0x0000DEAD`）。
- **LUKS2 主备报头与 JSON 区**：
  - `parseLuks2Header(data: []const u8): Luks2Header?` — 支持主头 `"LUKS\xBA\xBE"` 与备头 `"SKUL\xBA\xBE"`，严格校验 `version == 2`、`len >= 4096`、`hdr_size >= 16384` 且为 2 的幂。
  - `Luks2Header` 方法：`labelSlice()`、`csumAlgSlice()`、`uuidSlice()`、`subsystemSlice()`、`jsonAreaBytes()`、`extractJson(full_hdr)`。
  - `selectActiveLuks2(h1: Luks2Header?, h2: Luks2Header?): Luks2Header?` — 按 `seqid` 择优选取活跃报头。

## 示例

```cp
import { parseLuks2Header, selectActiveLuks2 } from "cap:devops";

fn inspectLuks2(primary_hdr: []const u8, secondary_hdr: []const u8) {
    const h1 = parseLuks2Header(primary_hdr);
    const h2 = parseLuks2Header(secondary_hdr);
    if (active := selectActiveLuks2(h1, h2)) {
        * = active.labelSlice();
        * = active.jsonAreaBytes();
        if (!active.is_secondary) {
            * = active.extractJson(primary_hdr);
        }
    }
}
```
