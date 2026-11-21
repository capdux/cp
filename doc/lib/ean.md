# `cap:ean` — ISO/IEC 15420 EAN-13 / EAN-8 / UPC-A 一维条码编解码库

纯 CP、零堆分配的 ISO/IEC 15420 一维商品条码模块编解码库，完整实现 L/G/R 7-bit 条空编码表、EAN-13 首位 6-bit 奇偶映射、GS1 Mod-10 校验、95 模块 EAN-13/UPC-A 与 67 模块 EAN-8 编码及支持自动倒置翻转扫描的解码器。

## 导入

```cp
import {
    EAN13_MODULES,
    EAN8_MODULES,
    UPCA_MODULES,
    EAN_DIGIT_MODULES,
    GUARD_OUTER,
    GUARD_CENTER,
    eanPatternL,
    eanPatternG,
    eanPatternR,
    ean13ParityMask,
    eanCheckDigit,
    encodeEan13Modules,
    decodeEan13Modules,
    encodeEan8Modules,
    decodeEan8Modules,
    encodeUpcAModules,
} from "cap:ean";
```

## 核心 API

- **协议常量与图案表**：
  - `EAN13_MODULES = 95`、`EAN8_MODULES = 67`、`UPCA_MODULES = 95`、`EAN_DIGIT_MODULES = 7`
  - `GUARD_OUTER = 0b101`、`GUARD_CENTER = 0b01010`
  - `eanPatternL(digit: u8): u8?`、`eanPatternG(digit: u8): u8?`、`eanPatternR(digit: u8): u8?`
  - `ean13ParityMask(first_digit: u8): u8?`
  - `eanCheckDigit(digits: []const u8): u8?` — 计算 GS1 Mod-10 校验数字（`0..9`）。
- **模块流编解码（`0`/`1` 字节流，支持倒置扫描）**：
  - `encodeEan13Modules(digits13: []const u8, out_modules: []u8): usize?`
  - `decodeEan13Modules(modules95: []const u8, out_digits13: []u8): usize?`
  - `encodeEan8Modules(digits8: []const u8, out_modules: []u8): usize?`
  - `decodeEan8Modules(modules67: []const u8, out_digits8: []u8): usize?`
  - `encodeUpcAModules(digits12: []const u8, out_modules: []u8): usize?`

## 示例

```cp
import std from "std";
import { encodeEan13Modules, decodeEan13Modules, EAN13_MODULES } from "cap:ean";

fn roundtripEan13(code: []const u8): bool {
    let mods: [95]u8 = std.mem.zeroes([95]u8);
    let out: [13]u8 = std.mem.zeroes([13]u8);
    const mlen = encodeEan13Modules(code, mods[0..]) ?? return false;
    const dlen = decodeEan13Modules(mods[0..mlen], out[0..]) ?? return false;
    return mlen == EAN13_MODULES && dlen == 13;
}
```
