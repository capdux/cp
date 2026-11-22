# `cap:srec` — 零分配 Motorola S-Record (S0–S9) 嵌入式固件编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:iot`，导入请使用 `import { ... } from "cap:iot"`。

> **模块路径**：`lib/srec/`（入口 `lib/srec/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 Motorola S-Record `.srec`/`.s19`/`.s28`/`.s37` 规范的 S0–S9 记录解析、大写编码、一补码校验和验证、多行流式迭代以及单遍固件汇总与计数校验。

---

## 一、导入方式

```cp
import {
    SrecType,
    SrecRecord,
    SrecStats,
    SrecIter,
    srecAddrBytes,
    srecChecksum,
    parseSrecLine,
    encodeSrecLine,
    summarizeSrec,
} from "cap:iot";
```

---

## 二、核心数据结构

| 类型 | 字段 / 枚举值 | 说明 |
| :--- | :--- | :--- |
| `SrecType: u8` | `s0_header=0`, `s1_data16=1`, `s2_data24=2`, `s3_data32=3`, `s5_count16=5`, `s6_count24=6`, `s7_start32=7`, `s8_start24=8`, `s9_start16=9`, `unknown=255` | Motorola S-Record 记录类型（S4 为保留类型，解析时拒绝） |
| `SrecRecord` | `rec_type`, `address: u32`, `data_len: u8`, `data: [255]u8`, `checksum: u8` | 栈上零分配记录结构；附 `dataSlice()`, `isData()`, `isTermination()` |
| `SrecStats` | `header_text_len`, `data_records`, `total_bytes`, `min_addr`, `max_addr`, `entry_addr: u32?`, `declared_count: u32?`, `count_matches: bool` | 单遍固件汇总结果（`min_addr..=max_addr` 闭区间） |

---

## 三、核心 API

- `srecAddrBytes(t: SrecType): u8` — 返回地址字段字节宽度（S0/S1/S5/S9 -> 2B；S2/S6/S8 -> 3B；S3/S7 -> 4B）。
- `srecChecksum(byte_count: u8, addr_bytes: u8, address: u32, payload: []const u8): u8` — 计算一补码校验和 `(~sum) & 0xFF`。
- `parseSrecLine(line: []const u8): SrecRecord?` — 严格校验 `'S'`+类型（拒绝 S4）、`byte_count >= addr_bytes + 1`、精确行长 `4 + byte_count * 2`、S5–S9 零负载与一补码校验和。
- `encodeSrecLine(rec_type: SrecType, address: u32, payload: []const u8, out: []u8): usize?` — 零分配输出标准大写 `S<n><LL><ADDR><DATA><CC>\n`。
- `SrecIter.init(text)` / `next(*_): SrecRecord?` — 迭代多行固件文本（自动忽略空行，兼容 `\r\n`/`\n`，遇非法行置 `had_error = true`）。
- `summarizeSrec(text: []const u8): SrecStats?` — 单遍汇总固件地址范围、字节数、入口地址并校验 S5/S6 声明计数与实际数据记录条数是否一致。

---

## 四、使用示例

```cp
import { SrecType, encodeSrecLine, parseSrecLine, summarizeSrec } from "cap:iot";

let buf: [64]u8 = [0; 64];
const payload: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
const n = encodeSrecLine(SrecType.s3_data32, 0x08000000, payload[0..], buf[0..]).?;
if (rec := parseSrecLine(buf[0..n])) {
    // rec.address == 0x08000000, rec.dataSlice() == {0xDE, 0xAD, 0xBE, 0xEF}
    * = rec;
}
```
