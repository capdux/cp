# CP 标准库：RFC 6350 / 2426 vCard 3.0 & 4.0 电子名片解析与生成引擎 (`cap:vcard`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

## 一、 架构总览与设计准则

`cap:vcard` 是 CP 语言官方内置的纯 CP **RFC 6350 / RFC 2426 vCard（3.0 & 4.0）电子名片通讯录 `.vcf` 批量解析与规范生成引擎**，**零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为个人助理 AI Agent、CRM 系统与邮件网关（配合 `cap:ical` / `cap:email`）设计。既支持 `import { ... } from "cap:doc"` 具名导入，也支持 `import cap from "cap"; cap.vcard.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/vcard/vcard.cp` | `// Source: cp/lib/vcard/vcard.cp (...)` | `VCardError`、`VCardVersion`、`StructuredName`、`TypedValue`、`VCardAddress`、`VCardContact`、`VCardCodec` |
| `lib/vcard/index.cp` | `// Source: cp/lib/vcard/index.cp (Unified Exports for cap:vcard)` | `cap:vcard` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与版本枚举

- **`VCardError`**：`InvalidFormat`、`MissingFn`、`UnsupportedVersion`、`UnbalancedCard`、`OutOfMemory`。
- **`VCardVersion`**：`V3_0`（`"3.0"`）、`V4_0`（`"4.0"`），提供 `toStr()` 与 `fromStr(s)`。

### 2. 数据结构

- **`StructuredName`**：`family`、`given`、`additional`、`prefix`、`suffix`（对应 `N:Last;First;Middle;Prefix;Suffix`）。
- **`TypedValue`**：`types: []string`、`value: string`（用于 `TEL` / `EMAIL` / `URL`，自动解析 `TYPE=CELL,WORK` 等多值参数）。
- **`VCardAddress`**：`types: []string`、`street`、`city`、`region`、`postal_code`、`country`（对应 `ADR` 分号结构化字段）。
- **`VCardContact`**：`version`、`uid`、`fn_name`（对应 `FN` 必填显示名）、`name: StructuredName?`、`org`、`title`、`phones: []TypedValue`、`emails: []TypedValue`、`urls: []TypedValue`、`addresses: []VCardAddress`、`note`，提供 `init(alloc)` 与 `deinit()`。

### 3. 编解码引擎 (`VCardCodec`)

- `VCardCodec.parseAll(alloc, vcf_text): ([]VCardContact) ! VCardError`：支持 CRLF/LF 75 字节行折行展开、`group.PROP` 属性组前缀剥离（如 `item1.TEL`）、`TYPE=` 逗号与分号多值拆分、转义分号/逗号正确切分 `N`/`ADR` 字段、`\\`/`\,`/`\;`/`\n` 反转义，严格校验 `BEGIN:VCARD`/`END:VCARD` 配对与必填 `FN`。
- `VCardCodec.parseOne(alloc, vcf_text): VCardContact ! VCardError`：解析单张名片。
- `VCardCodec.freeContacts(alloc, list)`：释放批量名片切片及全部内部字符串/数组内存。
- `VCardCodec.toVcf(alloc, contacts): ([]u8) ! VCardError` / `toVcfWithOptions(alloc, contacts, fold_75)`：按 RFC 6350 输出 `\r\n` 换行、标准转义与 75 字节 UTF-8 安全折行的 `.vcf` 文本。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { VCardCodec, VCardContact } from "cap:doc";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;
    const vcf =
        "BEGIN:VCARD\r\n" ++
        "VERSION:4.0\r\n" ++
        "FN:Alice Smith\r\n" ++
        "N:Smith;Alice;Marie;Dr.;PhD\r\n" ++
        "item1.TEL;TYPE=CELL,WORK:+1-555-0199\r\n" ++
        "EMAIL;TYPE=WORK:alice@example.com\r\n" ++
        "END:VCARD\r\n";

    let contacts = VCardCodec.parseAll(alloc, vcf) catch unreachable;
    defer VCardCodec.freeContacts(alloc, contacts);

    let out = cap.vcard.VCardCodec.toVcf(alloc, contacts) catch unreachable;
    defer alloc.free(out);
}
```
