# `cap:byte_size` — 人类可读字节大小解析与 SI/IEC 格式化库

> 参考来源：/data/dev/bun/src/bun_core/fmt.cp | 吸收日期：2026-11-22

## 概述

`cap:byte_size` 吸收自 Bun 核心格式化子系统（`src/bun_core/fmt.cp`），提供纯 CP、零堆分配、纯整数算术的字节容量解析器与 SI（1000 进制）/ IEC（1024 进制）规范格式化器，支持小数写法（如 `"1.5 GiB"`、`"2.25MB"`）并内置严格整数溢出防护，适用于配置文件限制（如 `maxBodySize`）、磁盘/内存配额与构建产物大小展示。

## 行为契约与安全边界

- **SI 与 IEC 双制式精确区分**：
  - **SI（十进制，底数 1000）**：`B`、`KB`、`MB`、`GB`、`TB`、`PB`；
  - **IEC（二进制，底数 1024）**：`B`、`KiB`、`MiB`、`GiB`、`TiB`、`PiB`；
  - 解析时自动忽略数值与单位间的可选空白并支持大小写不敏感单位匹配。
- **纯整数防浮点误差与溢出保护**：`parseByteSize` 全程使用定点数整数算术换算，避免 `f64` 在大数值下的精度丢失；遇到负值、空串、未知单位、非法小数点或超出 `u64::MAX` 的溢出均安全返回 `null`。
- **零堆分配输出**：`formatBytesIec` 与 `formatBytesSi` 写入调用方提供的栈缓冲 `out: []u8`，容量不足返回 `null`。

## 导出 API

| 函数签名 | 说明 |
| :--- | :--- |
| `parseByteSize(input: []const u8): u64?` | 将 `"512"`、`"10KB"`、`"1.5 GiB"`、`"2.25MB"` 等解析为精确 `u64` 字节数 |
| `formatBytesIec(bytes: u64, out: []u8): ([]const u8)?` | 以 1024 进制 IEC 单位（`B`, `KiB`, `MiB`, `GiB`, `TiB`, `PiB`）格式化 |
| `formatBytesSi(bytes: u64, out: []u8): ([]const u8)?` | 以 1000 进制 SI 单位（`B`, `KB`, `MB`, `GB`, `TB`, `PB`）格式化 |

## 使用示例

```cp
import { parseByteSize, formatBytesIec, formatBytesSi } from "cap:byte_size";

fn demo() {
    const bytes = parseByteSize("1.5 GiB").?; // 1610612736
    let buf1: [32]u8 = undefined;
    let buf2: [32]u8 = undefined;
    const iec = formatBytesIec(bytes, buf1[0..]).?; // "1.50 GiB"
    const si = formatBytesSi(bytes, buf2[0..]).?;   // "1.61 GB"
    * = iec;
    * = si;
}
```
