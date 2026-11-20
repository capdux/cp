# `cap:highway` — 纯 CP SIMD 向量化字节扫描、ASCII 校验与子串检索标准库

> **上游溯源**: `bun/src/highway/highway.cp` -> `cp/lib/highway/`  
> **模块路径**: `lib/highway/`  
> **导入协议**: `import { ... } from "cap:highway";` 或 `import cap from "cap"; cap.highway`

---

## 一、 架构概述与纯 CP 向量化设计

`cap:highway` 吸收了 Bun Google Highway 扫描层的核心能力，并彻底去除 C++ 外部绑定、`std.posix` 及 `@import("bun")` 依赖，完全基于纯 CP 与 Zig 原生 `@Vector(32, u8)` / `@Vector(16, u8)` 硬件向量类型实现：

1. **分级 SIMD 向量流水线 (32B -> 16B -> Scalar Tail)**：
   - 主循环优先以 32 字节（AVX2 / 双 NEON 寄存器）步长推进，随后处理 16 字节块，最后精确处理 `0..15` 字节非对齐尾部，确保任意长度切片均零越界且零对齐假设。
   - **位掩码提取**：通过 `@bitCast(chunk == @splat(target))` 将布尔向量转换为 `u32` / `u16` 整数掩码，结合 `@ctz`（正向首个命中）、`31 - @clz` / `15 - @clz`（反向末个命中）以及 `@popCount`（字节频次统计）实现单周期计算。
2. **首尾双字符 SIMD 过滤 (`indexOfSlice`)**：
   - 同时对 `needle[0]` 与 `needle[needle.len - 1]` 进行 32B/16B 向量广播比对并按位与归并，仅当首尾同时命中时才进入中间字节精确校验，大幅降低子串搜索的回溯开销。
3. **无分支 ASCII 转换与快速 UTF-8 校验**：
   - `toLowerAscii` / `toUpperAscii` / `replaceByte` 使用向量范围掩码与 `@select(u8, mask, ...)` 无分支完成批量转换。
   - `isUtf8ValidFast` 先通过 32B/16B SIMD 跳过纯 ASCII 前缀（若全为 ASCII 则 `O(N/32)` 直接返回 `true`），仅当遇到首个 `>= 0x80` 字节时回退到 `std.unicode.utf8ValidateSlice`。

---

## 二、 公开 API 列表

| 类别 | 函数签名 | 说明 |
| :--- | :--- | :--- |
| **单字节正向扫描** | `indexOfByte(haystack: []const u8, needle: u8): usize?` | 返回首个 `needle` 下标，未找到返回 `null` |
| **单字节反向扫描** | `lastIndexOfByte(haystack: []const u8, needle: u8): usize?` | 从末尾反向查找最后一个 `needle` 下标 |
| **多字节集合扫描** | `indexOfAnyByte(haystack: []const u8, needles: []const u8): usize?` | 查找 `needles` 中任意字节首次出现位置（特化 1..4 字节快路径） |
| **字节计数** | `countByte(haystack: []const u8, target: u8): usize` | `@popCount` 统计 `target` 出现次数 |
| **差异与前缀** | `indexOfNeq(a: []const u8, b: []const u8): usize?` | 返回首个差异字节下标（前缀关系返回 `min(a.len, b.len)`，完全相等返回 `null`） |
| **公共前缀长度** | `commonPrefixLen(a: []const u8, b: []const u8): usize` | 计算 `a` 与 `b` 最长公共前缀字节数 |
| **子串搜索** | `indexOfSlice(haystack: []const u8, needle: []const u8): usize?` | 首尾双字符 SIMD 过滤 + 校验子串搜索 |
| **ASCII 校验** | `isAscii(bytes: []const u8): bool` | 检查所有字节是否均 `< 0x80` |
| **快速 UTF-8 校验** | `isUtf8ValidFast(bytes: []const u8): bool` | SIMD 纯 ASCII 快路径 + `std.unicode.utf8ValidateSlice` 回退 |
| **控制字符检测** | `containsControlChars(bytes: []const u8): bool` | 检测是否包含 `< 0x20` 的 ASCII 控制符 |
| **大小写转换** | `toLowerAscii(dst: []u8, src: []const u8)` / `toUpperAscii(dst: []u8, src: []const u8)` | SIMD 无分支 ASCII 大小写转换 |
| **原地字节替换** | `replaceByte(buf: []u8, old_b: u8, new_b: u8)` | SIMD `@select` 原地替换所有 `old_b` 为 `new_b` |

---

## 三、 使用示例

```typescript
import cap from "cap";
import { indexOfSlice, isUtf8ValidFast, toLowerAscii } from "cap:highway";

fn demo(): void {
    const line = "GET /v1/models HTTP/1.1\r\n";
    const pos = indexOfSlice(line, "HTTP/1.1"); // 15
    const ok = isUtf8ValidFast(line);           // true

    let lower: [25]u8 = undefined;
    toLowerAscii(&lower, line);
    const has_ctrl = cap.highway.containsControlChars(line); // true ('\r', '\n')
}
```
