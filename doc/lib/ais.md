# `cap:ais` — 纯 CP ITU-R M.1371 海事船舶 AIS 6-bit 装甲比特流零分配解码库

## 概述

`cap:ais` 提供纯 CP、零外部依赖、零堆分配的 ITU-R M.1371 / IEC 61162-1 海事船舶自动识别系统（AIS）解码器：

- **NMEA 封装句校验**：解析 `!AIVDM` / `!AIVDO` 句子，严格验证 `*HH` 异或校验和、分片号（`1 <= frag_num <= frag_count`）与填充位数（`fill_bits <= 5`）；
- **6-bit ASCII Armor 零分配比特读取器**：`AisBitReader` 直接基于未解压的装甲 ASCII 切片按位流读取无符号整型、二补码符号扩展有符号整型（经纬度/转向率 ROT）与 6-bit AIS 字符串（自动剥离末尾 `@` 与空格），零额外堆/栈解码缓冲；
- **高频报文结构化解码**：完整支持 Types 1/2/3（Class A 位置报告，168 bits）与 Type 18（Class B 位置报告，168 bits），严格处理经纬度不可用哨兵值（`181*600000` / `91*600000`）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `AIS_LON_UNAVAILABLE` / `AIS_LAT_UNAVAILABLE` | 经纬度不可用原始整数哨兵常量 (`108600000` / `54600000`) |
| `AisSentence` / `parseAisSentence(line): AisSentence?` | 解析 `!AIVDM` / `!AIVDO` 封装句并验证 XOR 校验和 |
| `aisUnarmorChar(ch): u8?` | AIS 6-bit 装甲字符逆映射（`48..87` -> `-48`，`96..119` -> `-56`） |
| `aisMessageType(payload): u8?` | 快速读取首 6-bit AIS 报文类型编号 |
| `AisBitReader` | 零分配位流读取器：`init(payload, fill_bits)`、`remainingBits()`、`readU32(bits)`、`readI32(bits)`、`readSixbitString(char_count, out)` |
| `AisPositionReport` / `parseAisPositionReport(payload, fill_bits): AisPositionReport?` | 解码 Class A Types 1/2/3 动态位置报告（MMSI、航速、经纬度、航向、船首向、时间戳等） |
| `AisClassBPosition` / `parseAisClassBPosition(payload, fill_bits): AisClassBPosition?` | 解码 Class B Type 18 标准位置报告 |

## 示例

```cp
import { parseAisSentence, parseAisPositionReport } from "cap:ais";

fn decodeVesselMmsi(line: []const u8): u32 {
    const sent = parseAisSentence(line) ?? return 0;
    const pos = parseAisPositionReport(sent.payload, sent.fill_bits) ?? return 0;
    return pos.mmsi;
}
```
