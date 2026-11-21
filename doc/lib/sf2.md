# `cap:sf2` — 纯 CP SoundFont 2.0 `.sf2` RIFF `sfbk` 音频音色库零分配解析库

## 概述

`cap:sf2` 提供纯 CP、零外部依赖、零堆分配的 SoundFont 2.01 / 2.04（`.sf2` 小端 RIFF `sfbk`）解析器：

- **RIFF `sfbk` 顶层校验**：严格校验 12 字节 RIFF 头、`"sfbk"` 标识与 `8 + size <= data.len` 边界；
- **`LIST/INFO` 元数据零拷贝**：解析 `ifil` 版本与 `isng`/`INAM`/`irom`/`ICRD`/`IENG`/`IPRD`/`ICOP`/`ICMT`/`ISFT` 全字段（自动剥离末尾 `\0`）；
- **`LIST/sdta` 与 `LIST/pdta` 分段提取**：零拷贝切片 `smpl`（16-bit PCM）、可选 `sm24`（24-bit 低 8 位扩展）与 Hydra 表（`phdr`, `pbag`, `inst`, `ibag`, `shdr`），严格校验记录对齐；
- **Hydra 三表零分配迭代**：`Sf2PresetIter`（38B）、`Sf2InstIter`（22B）、`Sf2SampleIter`（46B）自动剔除末尾 `EOP`/`EOI`/`EOS` 哨兵条目，`Sf2Sample.samplePcmSlice(sections)` 安全零拷贝返回 16-bit PCM 字节区。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SF2_PRESET_HEADER_SIZE` (`38`), `SF2_INST_HEADER_SIZE` (`22`), `SF2_SAMPLE_HEADER_SIZE` (`46`), `SF2_BAG_SIZE` (`4`), `SF2_GEN_SIZE` (`4`) | SoundFont 2.0 Hydra 记录字节长度常量 |
| `isSf2(data): bool` | 校验 12B RIFF `sfbk` 头与边界 |
| `Sf2Version` / `Sf2Info` / `Sf2Sections` / `parseSf2(data): Sf2Sections?` | 解析顶层 `INFO`/`sdta`/`pdta` 全分段 |
| `Sf2Preset` / `Sf2PresetIter` | `phdr`（38B）预设迭代器（自动跳过末尾 `EOP`） |
| `Sf2Instrument` / `Sf2InstIter` | `inst`（22B）乐器迭代器（自动跳过末尾 `EOI`） |
| `Sf2Sample` / `Sf2SampleIter` | `shdr`（46B）采样头迭代器（自动跳过末尾 `EOS`），附 `samplePcmSlice(sections): ([]const u8)?` |

## 示例

```cp
import { parseSf2, Sf2PresetIter, Sf2SampleIter } from "cap:sf2";

fn countSamples(sf2_bytes: []const u8): usize {
    const sec = parseSf2(sf2_bytes) ?? return 0;
    * = sec.info.bank_name;

    let pit = Sf2PresetIter.init(sec.phdr);
    while (p := pit.next()) {
        * = p.name;
    }

    let sit = Sf2SampleIter.init(sec.shdr);
    let total_pcm_bytes: usize = 0;
    while (s := sit.next()) {
        if (pcm := s.samplePcmSlice(sec)) {
            total_pcm_bytes += pcm.len;
        }
    }
    return total_pcm_bytes;
}
```
