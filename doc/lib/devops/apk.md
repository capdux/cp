# `cap:apk` — Android APK ZIP EOCD & Signing Block v2/v3 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:devops`，导入请使用 `import { ... } from "cap:devops"`。

> **导入方式**：`import { findZipEocd, findApkSigningBlock, ApkSigPairIter, findApkSigPair, summarizeApkSigning, APK_SIG_V2_BLOCK_ID, APK_SIG_V3_BLOCK_ID } from "cap:devops";`  
> **内存模型**：100% 纯 CP 零堆分配，所有签名块与 ID-Value Pair 切片直接零拷贝指向原始 APK 缓冲区。

---

## 核心常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `ZIP_EOCD_SIG` | `0x06054B50` | ZIP EOCD 签名（`PK\x05\x06`） |
| `ZIP_EOCD_MIN_SIZE` | `22` | ZIP EOCD 最小字节数 |
| `ZIP_MAX_COMMENT_LEN` | `65535` | ZIP 注释最大长度 |
| `APK_SIG_BLOCK_MAGIC` | `"APK Sig Block 42"` | APK Signing Block 尾部 16B 魔数 |
| `APK_SIG_V2_BLOCK_ID` | `0x7109871A` | APK Signature Scheme v2 Block ID |
| `APK_SIG_V3_BLOCK_ID` | `0xF05368C0` | APK Signature Scheme v3 Block ID |
| `APK_SIG_V3_1_BLOCK_ID` | `0x1B93AD61` | APK Signature Scheme v3.1 Block ID |
| `APK_VERITY_PADDING_BLOCK_ID` | `0x42726577` | APK Verity 4KB 对齐填充块 ID |
| `APK_DEPENDENCY_INFO_BLOCK_ID` | `0x504B4453` | APK Dependency Info 块 ID |

---

## 核心数据结构与 API

- **`ApkZipEocd`**：`eocd_offset: usize`, `cd_entries: u16`, `cd_size: u32`, `cd_offset: u32`, `comment_len: u16`
- **`ApkSigningBlock`**：`block_offset: usize`, `block_size: u64`, `pairs_slice: []const u8`, `cd_offset: usize`
- **`ApkSigPair`**：`id: u32`, `value: []const u8`
- **`ApkSigSummary`**：`has_v2: bool`, `has_v3: bool`, `has_v3_1: bool`, `has_verity_padding: bool`, `pair_count: u32`

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `findZipEocd` | `fn findZipEocd(data: []const u8): ApkZipEocd?` | 从 EOF 反向定位 ZIP EOCD，严格校验 `comment_len` 精确到 EOF 且 `cd_offset + cd_size <= eocd_offset` |
| `findApkSigningBlock` | `fn findApkSigningBlock(data: []const u8): ApkSigningBlock?` | 定位并校验紧邻 CD 之前的 APK Signing Block（魔数 + 顶底 8B 长度一致 + 防下溢） |
| `ApkSigPairIter` | `ApkSigPairIter.init(block)` / `it.next(): ApkSigPair?` | 零分配遍历 ID-Value Pairs（`[8B len >= 4, 4B id, (len-4)B value]`），越界时置 `malformed = true` |
| `findApkSigPair` | `fn findApkSigPair(block: ApkSigningBlock, id: u32): ([]const u8)?` | 按 ID 零分配查找对应签名块载荷 |
| `summarizeApkSigning` | `fn summarizeApkSigning(data: []const u8): ApkSigSummary?` | 汇总 v2/v3/v3.1/Verity Padding 存在性与总 pair 数（畸形返回 `null`） |

---

## 使用示例

```typescript
import {
    findApkSigningBlock,
    findApkSigPair,
    summarizeApkSigning,
    APK_SIG_V2_BLOCK_ID,
} from "cap:devops";

fn inspectApk(apk_bytes: []const u8) {
    if (summary := summarizeApkSigning(apk_bytes)) {
        * = summary.has_v2;
        * = summary.has_v3;
    }
    if (block := findApkSigningBlock(apk_bytes)) {
        if (v2_payload := findApkSigPair(block, APK_SIG_V2_BLOCK_ID)) {
            * = v2_payload;
        }
    }
}
```
