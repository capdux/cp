# `cap:nmea` — 零分配 NMEA 0183 GPS/GNSS 导航语句与帧流解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:geo`，导入请使用 `import { ... } from "cap:geo"`。

> **模块路径**：`lib/nmea/`（入口 `lib/nmea/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 NMEA 0183 XOR 校验和计算与验证、通用语句与空字段保留迭代、`ddmm.mmmm`/`dddmm.mmmm` 十进制度数坐标换算、`GGA`/`RMC`/`VTG` 核心语句结构化解码以及串口多行帧流噪声过滤。

---

## 一、导入方式

```cp
import {
    NmeaSentence,
    NmeaGga,
    NmeaRmc,
    NmeaVtg,
    NmeaFieldIter,
    NmeaLineIter,
    nmeaChecksum,
    verifyNmeaSentence,
    parseNmeaSentence,
    parseNmeaCoord,
    parseNmeaGga,
    parseNmeaRmc,
    parseNmeaVtg,
} from "cap:geo";
```

---

## 二、核心数据结构

| 类型 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `NmeaSentence` | `start_char: u8`, `talker: []const u8`, `sentence_type: []const u8`, `fields_slice: []const u8`, `checksum: u8`; `fields(): NmeaFieldIter` | 通用 NMEA 0183 零拷贝语句视图（支持 `$` 与 `!` 如 AIS `!AIVDM`） |
| `NmeaGga` | `talker`, `utc_time`, `lat`, `lon`, `has_fix`, `fix_quality`, `satellites`, `hdop`, `altitude_m` | GGA 定位质量、星数、HDOP 与海拔解码结果 |
| `NmeaRmc` | `talker`, `utc_time`, `is_active`, `lat`, `lon`, `speed_knots`, `course_deg`, `date_ddmmyy`; `speedKmh()`, `speedMs()` | RMC 推荐最小定位与航速/航向解码结果 |
| `NmeaVtg` | `talker`, `true_track_deg`, `mag_track_deg`, `speed_knots`, `speed_kmh` | VTG 地面航向与速度解码结果 |

---

## 三、核心 API

### 1. XOR 校验和与验证
- `nmeaChecksum(body: []const u8): u8` — 计算 `$`/`!` 与 `*` 之间所有字节的 XOR 校验和。
- `verifyNmeaSentence(line: []const u8): bool` — 严格校验起始符 `$`/`!`、`*HH` 十六进制校验和及无杂散分隔符（兼容 `\r\n` / `\n`）。

### 2. 通用解析与字段/帧流迭代
- `parseNmeaSentence(line: []const u8): NmeaSentence?` — 验证并零拷贝切分 `talker` (2B)、`sentence_type` (3B) 与 `fields_slice`。
- `NmeaFieldIter.init(fields_slice)` / `next(*_): ([]const u8)?` — 按 `','` 切分字段，严格保留空字段 `""`。
- `NmeaLineIter.init(buf)` / `next(*_): NmeaSentence?` — 从多行串口/日志缓冲区零分配迭代所有合法 NMEA 语句，自动过滤噪声与校验失败行。

### 3. 坐标换算与结构化解码
- `parseNmeaCoord(dm_str: []const u8, hemi: []const u8): f64?` — 将 `ddmm.mmmm` / `dddmm.mmmm` + `N`/`S`/`E`/`W` 换算为十进制度数（南/西取负，严格校验范围）。
- `parseNmeaGga(line)` / `parseNmeaRmc(line)` / `parseNmeaVtg(line)` — 解析 GGA、RMC 与 VTG 核心导航语句。

---

## 四、使用示例

```cp
import { parseNmeaGga, parseNmeaRmc } from "cap:geo";

const gga_line = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
if (gga := parseNmeaGga(gga_line)) {
    // gga.lat ≈ 48.1173, gga.lon ≈ 11.516667, gga.altitude_m == 545.4, gga.satellites == 8
    * = gga;
}

const rmc_line = "$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*74";
if (rmc := parseNmeaRmc(rmc_line)) {
    const kmh = rmc.speedKmh(); // 22.4 * 1.852
    const ms = rmc.speedMs();   // 22.4 * 0.5144444
    * = kmh;
    * = ms;
}
```
