# `cap:uf2` — Microsoft UF2 (USB Flashing Format) 零分配固件编解码库

> **导入方式**：`import { isUf2, parseUf2Block, Uf2Iter, encodeUf2Block, summarizeUf2, UF2_FAMILY_RP2040, UF2_FLAG_FAMILY_ID_PRESENT } from "cap:uf2";`  
> **内存模型**：100% 纯 CP 零堆分配，解码块 `payload` 零拷贝指向原始 512 字节块缓冲，编码直接写入调用方 512 字节缓冲。

---

## 核心常量

- **结构尺寸**：`UF2_BLOCK_SIZE = 512`、`UF2_MAX_PAYLOAD = 476`
- **三重魔数**：`UF2_MAGIC_START0 = 0x0A324655` (`"UF2\n"`)、`UF2_MAGIC_START1 = 0x9E5D5157`、`UF2_MAGIC_END = 0x0AB16F30`
- **标志位**：`UF2_FLAG_NOT_MAIN_FLASH`、`UF2_FLAG_FILE_CONTAINER`、`UF2_FLAG_FAMILY_ID_PRESENT`、`UF2_FLAG_MD5_PRESENT`、`UF2_FLAG_EXTENSION_TAGS`
- **MCU Family ID**：`UF2_FAMILY_RP2040`、`UF2_FAMILY_RP2350_ARM_S`、`UF2_FAMILY_RP2350_RISCV`、`UF2_FAMILY_ESP32S2`、`UF2_FAMILY_ESP32S3`、`UF2_FAMILY_STM32F4`、`UF2_FAMILY_NRF52840`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isUf2` | `fn isUf2(data: []const u8): bool` | 校验长度为 512 正整数倍且首块三重魔数匹配 |
| `Uf2Block` | 含 `hasFamilyId()`, `familyId(): u32?`, `isNotMainFlash()`, `isFileContainer()` | 单个 512B 块的零拷贝解码视图 |
| `parseUf2Block` | `fn parseUf2Block(block_512: []const u8): Uf2Block?` | 严格校验 512B 长度、三重魔数、`payload_size <= 476`、`num_blocks > 0 && block_no < num_blocks` |
| `Uf2Iter` | `Uf2Iter.init(data)` / `it.next(): Uf2Block?` | 零分配逐块迭代完整 UF2 镜像 |
| `encodeUf2Block` | `fn encodeUf2Block(flags, target_addr, block_no, num_blocks, family_or_file_size, payload, out_512): bool` | 编码 512B 标准 UF2 块（自动清零余下负载并写尾魔数） |
| `summarizeUf2` | `fn summarizeUf2(data: []const u8): Uf2Summary?` | 单遍统计 `block_count`, `total_payload_bytes`, `min_addr`, `max_addr`, `family_id`, `is_contiguous` |

---

## 使用示例

```typescript
import {
    UF2_BLOCK_SIZE,
    UF2_FLAG_FAMILY_ID_PRESENT,
    UF2_FAMILY_RP2040,
    encodeUf2Block,
    summarizeUf2,
} from "cap:uf2";

fn buildSingleBlock(payload: []const u8) {
    let block: [UF2_BLOCK_SIZE]u8 = undefined;
    if (encodeUf2Block(UF2_FLAG_FAMILY_ID_PRESENT, 0x10000000, 0, 1, UF2_FAMILY_RP2040, payload, block[0..])) {
        if (summary := summarizeUf2(block[0..])) {
            * = summary.min_addr;
            * = summary.is_contiguous;
        }
    }
}
```
