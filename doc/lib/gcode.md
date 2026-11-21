# `cap:gcode` — CNC / 3D 打印 G-Code 零分配解析与刀路统计库

> **导入方式**：`import { gcodeChecksum, verifyGcodeChecksum, GcodeWordIter, parseGcodeLine, GcodeIter, summarizeGcode } from "cap:gcode";`  
> **内存模型**：100% 纯 CP 零堆分配，兼容 RS-274 / Marlin / Klipper / RepRap 方言。

---

## 核心数据结构

- **`GcodeWord`**：`letter: u8`（大写 `'A'..'Z'`）, `value: f64`, `raw_value: []const u8`
- **`GcodeLine`**：`command_letter: u8`, `command_code: i32`, `has_command: bool`, `line_number: u32?`, `x`/`y`/`z`/`e`/`f`/`s` (`f64`) 与对应 `has_*` (`bool`), `comment: []const u8`
- **`GcodeStats`**：`command_count`, `move_count`, `min_x`/`max_x`/`min_y`/`max_y`/`min_z`/`max_z`, `total_extrusion_e: f64`, `layer_changes: u32`, `is_metric: bool`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `gcodeChecksum` | `fn gcodeChecksum(prefix: []const u8): u8` | 计算 RepRap/Marlin `*` 前字节的逐字节 XOR 校验和 |
| `verifyGcodeChecksum` | `fn verifyGcodeChecksum(line: []const u8): bool` | 校验 `N<line> ... *<xor>` 行校验和 |
| `GcodeWordIter` | `GcodeWordIter.init(line)` / `it.next(): GcodeWord?` | 零分配解析单行参数词，自动跳过 `;` / `(...)` 注释与 `*checksum`，兼容紧邻紧凑写法如 `G1X10.5Y-2.25F1500` |
| `parseGcodeLine` | `fn parseGcodeLine(line: []const u8): GcodeLine` | 将单行 G-Code 解析为结构化指令 |
| `GcodeIter` | `GcodeIter.init(program)` / `it.next(): GcodeLine?` | 逐行迭代整份 G-Code 程序（跳过纯空行与纯注释行，兼容 `\r\n`/`\n`） |
| `summarizeGcode` | `fn summarizeGcode(program: []const u8): GcodeStats` | 单遍追踪 `G90`/`G91`、`M82`/`M83`、`G92`、`G20`/`G21`，计算真实 3D 包围盒与累计挤出量 |

---

## 使用示例

```typescript
import { parseGcodeLine, summarizeGcode } from "cap:gcode";

fn analyzePrint(program: []const u8) {
    const first = parseGcodeLine("G1X10.5Y-2.25Z0.2E1.5F1800 ; first layer");
    * = first.x;
    const stats = summarizeGcode(program);
    * = stats.total_extrusion_e;
    * = stats.layer_changes;
}
```
