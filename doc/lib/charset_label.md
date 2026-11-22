# `cap:charset_label` — WHATWG Encoding 字符集标签规范化与 BOM 嗅探库

> 参考来源：/data/dev/bun/src/runtime/webcore/encoding.cp | 吸收日期：2026-11-22

## 概述

`cap:charset_label` 吸收自 Bun WebCore 编码子系统（`src/runtime/webcore/encoding.cp`）并遵循 WHATWG Encoding Standard §4.2，提供纯 CP、零外部依赖、零堆分配的 HTTP `Content-Type; charset=...` 与 HTML `<meta charset>` 标签规范化映射及字节序标记（BOM）嗅探能力。

## 规范契约与行为边界

- **WHATWG 标签折叠规则**：
  - 解析前自动裁剪首尾 ASCII 空白（SP、HTAB、LF、FF、CR），再执行 ASCII 大小写不敏感匹配；
  - 遵循 WHATWG 别名归一：例如 `"utf8"` / `"unicode-1-1-utf-8"` → `utf8`；`"latin1"` / `"iso-8859-1"` / `"ascii"` / `"us-ascii"` → `windows1252`；`"gb2312"` / `"gb_2312"` / `"chinese"` → `gbk`；`"sjis"` / `"ms932"` → `shift_jis`。
- **ASCII 超集与 UTF-8 兼容判定**：
  - `isAsciiSuperset(kind)`：除 `utf16le` 与 `utf16be` 外均为 ASCII 超集（`0x00..0x7F` 字节保持原意，可直接安全扫描 ASCII 分隔符）；
  - `isUtf8Compatible(label)`：快速判定传入标签是否映射到 `utf8`。
- **BOM 嗅探优先级（`sniffBomCharset`）**：
  - `EF BB BF`（3 字节）→ `CharsetKind.utf8`；
  - `FE FF`（2 字节）→ `CharsetKind.utf16be`；
  - `FF FE`（2 字节）→ `CharsetKind.utf16le`；
  - 不足对应字节数或无匹配返回 `null`。

## 导入方式

```cp
import {
    CharsetKind,
    resolveCharsetLabel,
    canonicalCharsetName,
    isUtf8Compatible,
    isAsciiSuperset,
    sniffBomCharset,
} from "cap:charset_label";
```

## 核心类型与 API

- `CharsetKind`：`utf8`、`utf16le`、`utf16be`、`windows1252`、`iso8859_2`、`gbk`、`gb18030`、`big5`、`shift_jis`、`euc_jp`、`euc_kr`、`koi8_r`、`windows1251`、`ibm866`。
- `resolveCharsetLabel(label: []const u8): CharsetKind?`：去首尾 ASCII 空白、大小写无关解析 WHATWG 主流别名，未知标签返回 `null`。
- `canonicalCharsetName(kind: CharsetKind): []const u8`：返回标准 WHATWG 规范名称（如 `"utf-8"`、`"windows-1252"`、`"gbk"`）。
- `isUtf8Compatible(label: []const u8): bool` 与 `isAsciiSuperset(kind: CharsetKind): bool`。
- `sniffBomCharset(bytes: []const u8): CharsetKind?`：嗅探 UTF-8 / UTF-16BE / UTF-16LE BOM。

## 使用示例

```cp
import { resolveCharsetLabel, canonicalCharsetName, sniffBomCharset, isAsciiSuperset } from "cap:charset_label";

fn example() {
    if (k := resolveCharsetLabel("  GB2312  ")) {
        * = canonicalCharsetName(k); // "gbk"
        * = isAsciiSuperset(k);      // true
    }
    const bom: [3]u8 = [0xEF, 0xBB, 0xBF];
    * = sniffBomCharset(&bom); // CharsetKind.utf8
}
```
