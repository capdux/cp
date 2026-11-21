# `cap:dmg` — 零分配 Apple UDIF (`.dmg`) 磁盘镜像解析库

> **模块路径**：`lib/dmg/`（导入方式：`import { ... } from "cap:dmg";`）  
> **特性**：100% 纯 CP、零堆分配、零拷贝提取 XML plist 与数据分叉切片、全量防溢出与边界保护。

---

## 一、 核心常量

| 常量名 | 值 | 说明 |
| :--- | :--- | :--- |
| `DMG_KOLY_MAGIC` | `"koly"` | UDIF 512B 尾部块魔数（大端 `0x6B6F6C79`） |
| `DMG_KOLY_SIZE` | `512` | UDIF `koly` 尾部块固定字节数 |
| `DMG_MISH_MAGIC` | `"mish"` | BLKX 分区表头魔数（大端 `0x6D697368`） |
| `DMG_MISH_HEADER_SIZE` | `204` | `mish` 表头固定字节数 |
| `DMG_STRIPE_SIZE` | `40` | 单条 BLKX 条带描述符字节数 |
| `BLKX_*` | `ZERO` / `RAW` / `IGNORE` / `ADC` / `ZLIB` / `BZ2` / `LZFSE` / `LZMA` / `COMMENT` / `LAST` | 常见 BLKX 块压缩与控制类型常量 |

---

## 二、 核心数据结构与方法

### `DmgKolyHeader`
解析 `.dmg` 末尾 512 字节大端 `koly` 块：
- 核心字段：`version`、`header_size`、`flags`、`running_data_fork_offset`、`data_fork_offset`、`data_fork_length`、`rsrc_fork_offset`、`rsrc_fork_length`、`segment_number`、`segment_count`、`data_checksum_type`、`data_checksum_size`、`xml_offset`、`xml_length`、`master_checksum_type`、`image_variant`、`sector_count`
- 方法：
  - `xmlSlice(data: []const u8): ([]const u8)?` — 零拷贝提取内嵌 XML plist 切片（防溢出越界）
  - `dataForkSlice(data: []const u8): ([]const u8)?` — 零拷贝提取 Data Fork 切片（防溢出越界）
  - `uncompressedBytes(): u64` — 计算 `sector_count * 512`（溢出时饱和至 `u64::MAX`）

### `DmgMishHeader`、`DmgBlkStripe` & `DmgStripeIter`
- `parseDmgMish(data: []const u8): DmgMishHeader?` — 解析 204B 大端 `mish` 表头（`version`, `sector_start`, `sector_count`, `data_offset`, `buffers_needed`, `block_descriptors`, `stripe_count`）
- `DmgStripeIter.init(data)` / `DmgStripeIter.fromHeader(data, header)` / `next(*_): DmgBlkStripe?` — 迭代每条 40B `DmgBlkStripe`（`entry_type`, `sector_start`, `sector_count`, `comp_offset`, `comp_length`）

---

## 三、 使用示例

```typescript
import {
    isDmg,
    parseDmgKoly,
    parseDmgMish,
    DmgStripeIter,
    BLKX_LAST,
} from "cap:dmg";

fn inspectDmg(image: []const u8, blkx: []const u8) {
    if (isDmg(image)) {
        if (koly := parseDmgKoly(image)) {
            if (xml := koly.xmlSlice(image)) {
                * = xml.len;
            }
            * = koly.uncompressedBytes();
        }
    }
    if (mish := parseDmgMish(blkx)) {
        let it = DmgStripeIter.fromHeader(blkx, mish);
        while (stripe := it.next()) {
            if (stripe.entry_type == BLKX_LAST) break;
        }
    }
}
```
