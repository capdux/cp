# `cap:string` — 工业级字符串引擎、Unicode 码表与可变缓冲标准库

> **上游溯源**: `/data/dev/bun/src/string/` & `/data/dev/bun/src/unicode/` (`immutable.cp`, `immutable/unicode.cp`, `wtf.cp`, `StringBuilder.cp`, `MutableString.cp`, `SmolStr.cp`, `HashedString.cp`, `StringJoiner.cp`, `PathString.cp`, `escapeRegExp.cp`, `js_parser/lexer/identifier.cp`, `uucode`)  
> **模块路径**: `lib/string/`  
> **导入协议**: `import { strings, Strings, StringBuilder, MutableString, Unicode, CodepointIterator, Wtf8, Utf8, Utf16, SmolStr, HashedString, StringJoiner, CaseConverter, StringEscape, SimdScanner, wcwidth, stringWidth, ... } from "cap:string";`

---

## 一、 架构概述与模块文件拓扑

`cap:string` 完整吸收 Bun 生产级字符串与 Unicode 子系统，针对上层编译器、Shell、包管理、HTTP/CSS/JSON 提供了全套高频匹配、转码、构建与 Unicode 分类能力（零 `@import("bun")` 依赖）：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/string/utils.cp` | `// Source: /data/dev/bun/src/string/immutable.cp` | `Strings` / `strings` 门面与导出函数：`eql`/`eqlLong`/`eqlComptime`/`eqlCaseInsensitiveASCII*`/`hasPrefix*`/`hasSuffix*`/`startsWithCaseInsensitiveASCII`/`endsWithCaseInsensitiveASCII`/`indexOfChar*`/`lastIndexOfChar`/`indexOf`/`lastIndexOf`/`indexOfAny`/`indexOfNotChar`/`contains*`/`countChar`/`order`/`cmpStrings*`/`trimLeft`/`trimRight`/`trimWhitespace`/`trimPrefix`/`trimSuffix`/`split`/`splitScalar`/`concat`/`concatBuf`/`join`/`replaceAll*`/`repeat`/`repeating*`/`formatZigString`/`isAllASCII`/`isASCII`/`toLowerCaseASCII`/`toUpperCaseASCII` |
| `lib/string/utf.cp` | `// Source: /data/dev/bun/src/string/wtf.cp` | `Utf8`、`Utf16`、`Wtf8`（孤立代理对兼容）、`EncodeIntoResult`、`copyLatin1IntoUTF8`、`elementLengthLatin1IntoUTF8`、`copyUTF16IntoUTF8`、`elementLengthUTF16IntoUTF8`、`convertUTF8toUTF16*`、`convertUTF16toUTF8*`、`utf8ByteSequenceLength`、`decodeUTF8`、`encodeUTF8`、`withoutUTF8BOM` |
| `lib/string/unicode.cp` | `// Source: /data/dev/bun/src/unicode/ & /data/dev/bun/src/js_parser/lexer/identifier.cp` | `Unicode`、`CodepointIterator`、`CodepointCursor`、`isWhitespaceCodepoint`/`isNewlineCodepoint`/`isLineTerminator`/`isIdentifierStart`/`isIdentifierContinue`/`isAlphabeticCodepoint`/`toLowerCodepoint`/`toUpperCodepoint`/`caseFoldCodepoint` |
| `lib/string/builder.cp` | `// Source: /data/dev/bun/src/string/StringBuilder.cp` | 两阶段零重分配 `StringBuilder`（支持 `init`/`initWithAllocator`/`initCapacity`/`count`/`countZ`/`count16`/`allocate`/`allocateIn`/`append`/`appendZ`/`append16`/`appendFmt`/`fmt`/`writable`/`allocatedSlice`/`toOwnedSlice`/`length`/`capacity`） |
| `lib/string/mutable_string.cp` | `// Source: /data/dev/bun/src/string/MutableString.cp` | `MutableString` 可增长字符串缓冲（`init`/`initEmpty`/`init2048`/`initCopy`/`ensureUnusedCapacity`/`growIfNeeded`/`append`/`appendChar*`/`appendInt`/`appendSignedInt`/`appendFmt`/`writableNBytes*`/`reset`/`toOwnedSlice*`） |
| `lib/string/escape.cp` | `// Source: /data/dev/bun/src/string/escapeRegExp.cp` | `StringEscape` 与顶层导出 `escapeHtml` / `escapeHtmlAlloc` / `escapeRegExp` / `escapeRegExpAlloc` / `escapeJs` / `escapeJsAlloc` |
| `lib/string/wcwidth.cp` | `// Source: /data/dev/bun/src/unicode/uucode_lib/src/x/config_x/wcwidth.cp` | `wcwidth`、`stringWidth`、`StringWidth` |
| `lib/string/grapheme.cp` | `// Source: /data/dev/bun/src/unicode/` | UAX #29 `GraphemeIterator`、`graphemeCount`、`Utf8Grapheme` |
| `lib/string/simd_scan.cp` | `// Source: /data/dev/bun/src/highway/` | 16 字节 `@Vector(16, u8)` `SimdScanner` |
| `lib/string/smol.cp` | `// Source: /data/dev/bun/src/string/SmolStr.cp` | `SmolStr` 15 字节栈内联短字符串优化 (SSO) |
| `lib/string/hashed.cp` | `// Source: /data/dev/bun/src/string/HashedString.cp` | `HashedString` FNV-1a 预计算哈希视图 |
| `lib/string/case.cp` | `// Source: /data/dev/bun/src/string/` | `CaseConverter`（snake/kebab/camel/Pascal/SCREAMING） |
| `lib/string/joiner.cp` | `// Source: /data/dev/bun/src/string/` | `StringJoiner` |
| `lib/string/path.cp` | `// Source: /data/dev/bun/src/string/` | `PathString` |
| `lib/string/index.cp` | `// Source: /data/dev/bun/src/string/` | `cap:string` 统一公开导出入口 |

---

## 二、 核心 API 使用示例

### 1. Bun `strings.*` 高频匹配、切分与连接

```typescript
const std = @import("std");
import { strings, StringBuilder, MutableString, Unicode, CodepointIterator, copyUTF16IntoUTF8 } from "cap:string";

pub fn demoPhaseAString(): !void {
    const gpa = std.heap.page_allocator;

    // 1. 匹配、查找、修剪与连接
    const ok = strings.startsWithCaseInsensitiveASCII("Content-Type: json", "content-type:");
    const pos = strings.indexOfAny("k=v;x", ";="); // 1
    let parts: [3][]const u8 = ["lib", "string", "index.cp"];
    const joined = try strings.join(gpa, "/", parts[0..3]);
    defer gpa.free(joined);
    * = ok; * = pos;

    // 2. MutableString & StringBuilder
    let ms = try MutableString.init2048(gpa);
    defer ms.deinit();
    try ms.append("id=");
    try ms.appendInt(100);

    let sb = StringBuilder.initWithAllocator(gpa);
    defer sb.release();
    sb.count("prefix:");
    sb.countZ("ok");
    try sb.allocateIn(gpa);
    * = sb.append("prefix:");
    * = sb.appendZ("ok");

    // 3. Unicode 与 CodepointIterator
    let it = CodepointIterator.init("α中🚀");
    while (true) {
        const cp_opt = it.next();
        if (cp_opt == null) break;
        * = Unicode.charWidth(cp_opt.?);
    }
}
```
