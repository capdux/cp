# `cap:spc` — 零分配 SNES SPC700 (`.spc`) 芯片音乐解析库

> **模块路径**：`lib/spc/`（入口 `lib/spc/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 SNES SPC700 v0.30 256B 文件头、CPU 初始寄存器、Text/Binary ID666 自动识别与标签解码、64KB RAM / 128B DSP / 64B Extra RAM 零拷贝切片以及 4 字节对齐的 `xid6` 扩展块迭代。

---

## 一、导入方式

```cp
import {
    SPC_MAGIC,
    SPC_HEADER_SIZE,
    SPC_RAM_OFFSET,
    SPC_RAM_SIZE,
    SPC_DSP_OFFSET,
    SPC_DSP_SIZE,
    SPC_EXTRA_RAM_OFFSET,
    SPC_EXTRA_RAM_SIZE,
    SPC_MIN_FILE_SIZE,
    SPC_XID6_MAGIC,
    SpcId666Format,
    SpcRegs,
    SpcHeader,
    SpcXid6Item,
    SpcXid6Iter,
    isSpc,
    parseSpcHeader,
} from "cap:spc";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| `SPC_MAGIC` | `"SNES-SPC700 Sound File Data v0.30"` | 33 字节文件魔数（后接 `0x1A 0x1A`） |
| `SPC_MIN_FILE_SIZE` | `66048` | 完整快照最小长度（256B 头 + 64KB RAM + 128B DSP + 64B 保留 + 64B Extra RAM） |
| `SpcId666Format: u8` | `none=0`, `text=1`, `binary=2` | 自动判别的 ID666 标签格式 |
| `SpcRegs` | `pc: u16`, `a`, `x`, `y`, `psw`, `sp` | SPC700 CPU 初始寄存器状态 |
| `SpcHeader` | `has_id666`, `version_minor`, `regs`, `id666_format`, `song_title`, `game_title`, `dumper_name`, `comments`, `artist`, `play_seconds`, `fade_ms`, `emulator_id` | 256B 头部视图；附 `ramSlice(data)`, `dspSlice(data)`, `extraRamSlice(data)` |
| `SpcXid6Item` | `id: u8`, `data_type: u8` (`0`/`1`/`4`), `int_val: u32`, `str_val: []const u8` | Extended ID666 (`xid6`) 4B 对齐子块条目 |

---

## 三、核心 API

- `isSpc(data: []const u8): bool` — 校验 `>= 256B`、33B 魔数及偏移 `33..35` 的 `[0x1A, 0x1A]`。
- `parseSpcHeader(data: []const u8): SpcHeader?` — 解析 CPU 寄存器并自动区分 Text 与 Binary ID666 提取元数据。
- `SpcXid6Iter.init(file_data)` / `next(*_): SpcXid6Item?` — 定位偏移 `66048` 处的 `"xid6"` 块并按 4 字节对齐迭代全部子块，严格防越界。

---

## 四、使用示例

```cp
import { parseSpcHeader, SpcXid6Iter } from "cap:spc";

fn inspectSpc(data: []const u8) {
    if (hdr := parseSpcHeader(data)) {
        if (ram := hdr.ramSlice(data)) {
            * = ram.len; // 65536
        }
        let xit = SpcXid6Iter.init(data);
        while (item := xit.next()) {
            * = item;
        }
    }
}
```
