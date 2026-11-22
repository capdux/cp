# `cap:fdt` — Devicetree Specification v0.4 (`.dtb`) 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

> **模块路径**: `lib/fdt/`  
> **导入协议**: `import { FDT_MAGIC, isFdt, parseFdt, FdtTokenKind } from "cap:sys";`  
> **动态挂载**: `import cap from "cap";` -> `cap.fdt.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:fdt` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 Devicetree Specification v0.4 Flattened Device Tree（`.dtb`）二进制设备树解析库：

1. **40B 头部与全区段边界严审**：
   - `parseFdt` 校验大端魔数 `0xD00DFEED`、`totalsize` 范围、`version >= 17 && last_comp_version <= 17`，并验证 `mem_rsvmap` / `dt_struct` / `dt_strings` 全区段 4 字节对齐与无整数溢出越界。
2. **Memory Reservation Map 零拷贝迭代**：
   - `FdtMemRsvIter` 按 16 字节步进读取 `(address, size)`，遇 `(0, 0)` 哨兵自动终止。
3. **Structure Block 单遍零拷贝 Token 迭代**：
   - `FdtTokenIter` 自动跳过 `FDT_NOP`、按 4 字节边界推进节点名与属性值、实时追踪嵌套深度 `depth`、按偏移零拷贝解析 Strings Block 属性名；遇到越界、未终止字符串、未配对节点或非法 Token 时置 `hasError()` 并终止。
   - `FdtToken` 内置 `asU32()`（大端 32-bit）、`asU64()`（大端 64-bit）与 `asString()`（校验并剥离尾 NUL）。

---

## 二、 核心 API 速查表 (`lib/fdt/fdt.cp`)

| 类别 | 符号 / 方法 | 签名与说明 |
| :--- | :--- | :--- |
| 协议常量 | 魔数与 Token | `FDT_MAGIC (0xD00DFEED)`, `FDT_HEADER_SIZE (40)`, `FDT_VERSION (17)`, `FDT_BEGIN_NODE (1)`, `FDT_END_NODE (2)`, `FDT_PROP (3)`, `FDT_NOP (4)`, `FDT_END (9)` |
| 数据模型 | `FdtHeader` | `totalsize`, `off_dt_struct`, `off_dt_strings`, `off_mem_rsvmap`, `version`, `last_comp_version`, `boot_cpuid_phys`, `size_dt_strings`, `size_dt_struct` |
| 数据模型 | `FdtMemRsv` & `FdtMemRsvIter` | `address: u64`, `size: u64`, `next(*_): FdtMemRsv?` |
| 数据模型 | `FdtTokenKind` & `FdtToken` | `begin_node=1`, `end_node=2`, `prop=3`, `end=9`；`asU32(): u32?`, `asU64(): u64?`, `asString(): ([]const u8)?` |
| 数据模型 | `FdtTokenIter` | `next(*_): FdtToken?`, `hasError(): bool` |
| 数据模型 | `FdtBlob` | `raw`, `header`, `memReservations()`, `tokens()`, `stringAt(nameoff)` |
| 解析函数 | 顶层入口 | `isFdt(data)`, `parseFdt(data)` |

---

## 三、 使用示例

```typescript
import { parseFdt, FdtTokenKind } from "cap:sys";

fn walkDtb(dtb_bytes: []const u8) {
    if (blob := parseFdt(dtb_bytes)) {
        let rsv_iter = blob.memReservations();
        while (rsv := rsv_iter.next()) {
            * = rsv.address;
        }
        let tok_iter = blob.tokens();
        while (tok := tok_iter.next()) {
            if (tok.kind == FdtTokenKind.prop) {
                * = tok.name;
            }
        }
    }
}
```
