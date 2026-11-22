# `cap:dls` — 零分配 MMA Downloadable Sounds (`.dls`) RIFF 音色库解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

> **模块路径**：`lib/dls/`（入口 `lib/dls/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 MMA DLS Level 1 & Level 2 RIFF 规范（小端序、2 字节偶对齐），支持 `colh`/`vers`/`LIST/INFO`/`ptbl` 头部解析、`LIST/lins` 旋律/鼓组乐器迭代以及 `LIST/wvpl` 波形池（`fmt `/`wsmp`/`data`）零拷贝提取。

---

## 一、导入方式

```cp
import {
    DLS_RIFF_MAGIC,
    DLS_FORM_TYPE,
    DLS_BANK_DRUM_FLAG,
    DlsVersion,
    DlsInfo,
    DlsHeader,
    DlsInstrument,
    DlsWave,
    DlsInstIter,
    DlsWaveIter,
    isDls,
    parseDls,
} from "cap:media";
```

---

## 二、核心数据结构

| 类型 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `DlsVersion` | `ms_version: u32`, `ls_version: u32`; `major(): u16`, `minor(): u16` | DLS `vers` 64-bit 版本号 |
| `DlsInfo` | `inam`, `icop`, `ieng`, `icmt`, `isft`, `icrd` | `LIST/INFO` 零拷贝元数据（自动去 `\0`） |
| `DlsHeader` | `instrument_count`, `version`, `has_version`, `info`, `lins_slice`, `wvpl_slice`, `ptbl_slice`, `cue_count`; `instruments()`, `waves()` | 顶层 DLS 容器视图 |
| `DlsInstrument` | `name`, `regions`, `bank`, `instrument_program`; `isDrum(): bool` | `LIST/ins ` 乐器项（`(bank & 0x80000000) != 0` 为鼓组） |
| `DlsWave` | `name`, `format_tag`, `channels`, `sample_rate`, `bits_per_sample`, `unity_note`, `fine_tune`, `attenuation`, `has_wsmp`, `pcm_data` | `LIST/wave` 波形与 PCM 零拷贝切片 |

---

## 三、核心 API

- `isDls(data: []const u8): bool` — 校验 12B RIFF `DLS ` 头且 `8 + riff_size <= data.len`。
- `parseDls(data: []const u8): DlsHeader?` — 零分配解析顶层块并定位 `lins`/`wvpl`/`ptbl` 切片。
- `DlsInstIter.init(lins_slice)` / `next(*_): DlsInstrument?` — 迭代全部 `LIST/ins ` 乐器定义。
- `DlsWaveIter.init(wvpl_slice)` / `next(*_): DlsWave?` — 迭代全部 `LIST/wave` 波形池音频项。

---

## 四、使用示例

```cp
import { parseDls } from "cap:media";

fn inspectDls(data: []const u8) {
    if (hdr := parseDls(data)) {
        let inst_it = hdr.instruments();
        while (inst := inst_it.next()) {
            * = inst.isDrum();
        }
        let wave_it = hdr.waves();
        while (w := wave_it.next()) {
            * = w.pcm_data;
        }
    }
}
```
