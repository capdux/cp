# `cap:vcd` — 零分配 IEEE 1364 VCD 硬件仿真波形解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

> **模块路径**：`lib/vcd/`（导入方式：`import { ... } from "cap:time";`）  
> **特性**：100% 纯 CP、零堆分配、零拷贝切片视图，支持完整 Verilator / Icarus Verilog VCD 头、信号层次定义与时间戳/标量/向量/实数值变流解析。

---

## 一、 核心枚举

- **`VcdTimeUnit`**：`s`, `ms`, `us`, `ns`, `ps`, `fs`, `unknown`
- **`VcdVarType`**：`wire`, `reg`, `integer`, `real`, `parameter`, `event`, `supply0`, `supply1`, `tri`, `unknown`
- **`VcdEventKind`**：`timestamp`, `scalar_change`, `vector_change`, `real_change`, `dumpvars`, `dumpon`, `dumpoff`, `dumpall`, `end`

---

## 二、 核心数据结构与函数

- **`VcdTimescale`** (`magnitude: u32`, `unit: VcdTimeUnit`) & **`parseVcdTimescale(text)`**
- **`VcdHeader`** (`date`, `version`, `timescale`, `var_count`, `body_offset`) & **`parseVcdHeader(data): VcdHeader?`**
- **`VcdVarDef`** (`var_type`, `width`, `id_code`, `reference`) & **`VcdVarIter.init(data)` / `next(*_)`**
- **`VcdEvent`** (`kind`, `timestamp`, `scalar_val`, `id_code`, `value`) & **`VcdValueIter.init(data, body_offset)` / `fromHeader(data, header)` / `next(*_)`**

---

## 三、 使用示例

```typescript
import {
    parseVcdHeader,
    VcdVarIter,
    VcdValueIter,
    VcdEventKind,
} from "cap:time";

fn inspectVcd(vcd: []const u8) {
    if (hdr := parseVcdHeader(vcd)) {
        let vars = VcdVarIter.init(vcd);
        while (v := vars.next()) {
            * = v.width;
        }
        let vals = VcdValueIter.fromHeader(vcd, hdr);
        while (ev := vals.next()) {
            if (ev.kind == VcdEventKind.timestamp) {
                * = ev.timestamp;
            }
        }
    }
}
```
