# `cap:rtf` — 零分配 Rich Text Format (RTF 1.9) 词法与纯文本提取库

> **模块路径**：`lib/rtf/`（导入方式：`import { ... } from "cap:rtf";`）  
> **特性**：100% 纯 CP、零堆分配、完整遵循 RTF 1.9 词法规范（控制字单空格分隔符、十六进制 `\'hh`、`\uN` UTF-8 与 `\ucN` 替换跳过、自动跳过 `\fonttbl`/`\colortbl`/`\*` 目的地组）。

---

## 一、 核心常量与枚举

- `RTF_MAGIC = "{\\rtf"`、`MAX_RTF_DEPTH = 64`
- `RtfTokenKind`：`group_start`, `group_end`, `control_word`, `control_symbol`, `hex_byte`, `text`
- `RtfCharset`：`unknown`, `ansi`, `mac`, `pc`, `pca`

---

## 二、 核心 API

- `isRtf(data: []const u8): bool` — 跳过前导空白校验 `"{\\rtf"` 前缀
- `RtfLexer.init(data)` / `next(*_): RtfToken?` — 零分配词法流解析
- `parseRtfHeader(data): RtfHeaderInfo?` — 提取 `version`、`charset`、`ansicpg`、`default_font`、`uc_skip`
- `extractRtfText(data: []const u8, out: []u8): usize?` — 零堆分配提取正文纯文本到 `out`（含嵌套组栈深度防护、括号平衡校验与输出缓冲区溢出防护）

---

## 三、 使用示例

```typescript
import { parseRtfHeader, extractRtfText } from "cap:rtf";

fn readRtfDoc(doc: []const u8) {
    if (hdr := parseRtfHeader(doc)) {
        * = hdr.ansicpg;
    }
    let out: [256]u8 = undefined;
    if (n := extractRtfText(doc, out[0..256])) {
        * = out[0..n].len;
    }
}
```
