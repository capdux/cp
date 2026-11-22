# `cap:igc` — FAI IGC 航空/滑翔机飞行记录仪零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

> **导入方式**：`import { isIgc, parseIgcHeader, parseIgcBRecord, IgcFixIter, summarizeIgc } from "cap:text";`  
> **内存模型**：100% 纯 CP 零堆分配，所有头字段零拷贝指向原始 IGC 缓冲区。

---

## 核心数据结构

- **`IgcHeader`**：`manufacturer`, `logger_id`, `date_ddmmyy`, `pilot`, `glider_type`, `glider_id`, `fr_type`, `gps_datum`（均 `[]const u8`）
- **`IgcFix`**：`hour`, `minute`, `second`, `lat_deg: f64`, `lon_deg: f64`, `validity: u8` (`'A'`/`'V'`), `pressure_alt_m: i32`, `gnss_alt_m: i32`，提供 `timeSeconds(): u32`
- **`IgcStats`**：`fix_count`, `valid_3d_count`, `min_alt_m`, `max_alt_m`, `duration_sec`（支持跨午夜 UTC 翻转）, `min_lat`, `max_lat`, `min_lon`, `max_lon`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isIgc` | `fn isIgc(data: []const u8): bool` | 校验首行 `A` 记录（`'A'` + 3B 厂商码 + 3B 记录仪 ID） |
| `parseIgcHeader` | `fn parseIgcHeader(data: []const u8): IgcHeader?` | 解析 `A` 与 `H` 头记录（兼容新版 `HFDTEDATE:DDMMYY,NN` 与旧版 `HFDTEDDMMYY`） |
| `parseIgcBRecord` | `fn parseIgcBRecord(line: []const u8): IgcFix?` | 解析 35+ 字节定宽 `B` 定位记录（含南纬/西经符号与负气压海拔） |
| `IgcFixIter` | `IgcFixIter.init(data)` / `it.next(): IgcFix?` | 零分配迭代全部 `B` 定位点（兼容 `\r\n`/`\n`） |
| `summarizeIgc` | `fn summarizeIgc(data: []const u8): IgcStats` | 单遍统计飞行时长、高度极值与 2D 经纬度包围盒 |

---

## 使用示例

```typescript
import { parseIgcHeader, summarizeIgc } from "cap:text";

fn inspectFlight(igc_bytes: []const u8) {
    if (hdr := parseIgcHeader(igc_bytes)) {
        * = hdr.pilot;
        * = hdr.date_ddmmyy;
    }
    const stats = summarizeIgc(igc_bytes);
    * = stats.max_alt_m;
    * = stats.duration_sec;
}
```
