# `cap:string` — 工业级字符串引擎、Unicode 字素簇与 SSO 内存优化标准库

> **上游溯源**: `/data/dev/bun/src/string/` & `/data/dev/bun/src/unicode/` (`SmolStr.cp`, `HashedString.cp`, `StringBuilder.cp`, `StringJoiner.cp`, `PathString.cp`, `wtf.cp`, `immutable.cp`, `escapeRegExp.cp`)  
> **模块路径**: `lib/string/`  
> **导入协议**: `import { SmolStr, HashedString, Utf8, Utf16, UnicodeWidth, Utf8Grapheme, CaseConverter, StringBuilder, StringJoiner, PathString, StringEscape, SimdScanner } from "cap:string";`

---

## 一、 架构概述与核心组件矩阵

`cap:string` 完整吸收自 Bun 高性能字符串与 Unicode 子系统（`bun/src/string` 与 `bun/src/unicode`），针对百万级并发 Agent 与高吞吐网络/编译工作负载提供了 10 大零分配或单次分配原语：

```text
lib/string/
├── index.cp         # 模块统一导出门面 (cap:string)
├── smol.cp          # SmolStr: 15 字节栈内联短字符串优化 (SSO) 容器
├── hashed.cp        # HashedString: FNV-1a 32-bit 预计算哈希 O(1) 字符串比较视图
├── utf.cp           # Utf8 / Utf16 / UnicodeWidth: UTF-8/UTF-16LE 编解码与终端显示宽度
├── grapheme.cp      # Utf8Grapheme: RFC 3629 校验与 UAX #29 扩展字素簇 (Emoji ZWJ) 分词计数
├── simd_scan.cp     # SimdScanner: 16 字节 @Vector(16, u8) 向量 ASCII 与字节扫描器
├── builder.cp       # StringBuilder: 两阶段 (Count -> Allocate -> Append) 零重分配构建器
├── joiner.cp        # StringJoiner: 动态缓冲拼接与切片数组分隔符连接器
├── case.cp          # CaseConverter: 零堆分配命名风格转换器 (snake/kebab/camel/Pascal/SCREAMING)
├── escape.cp        # StringEscape: 高速 HTML 实体转义与 RegExp 元字符转义
└── path.cp          # PathString: 轻量级零拷贝路径字符串视图
```

---

## 二、 核心数据结构深度剖析

### 1. `SmolStr` — 15 字节短字符串栈内联优化 (SSO) 内存布局

在编译器 AST 标识符、HTTP 头部键名与 JSON 字段名中，超过 85% 的字符串长度不超过 15 字节。`SmolStr` 采用双模式内存布局：

| 字段名 | 类型 | 偏移与用途说明 |
| :--- | :--- | :--- |
| `inline_buf` | `[15]u8` | 当 `len <= 15` (`SMOL_MAX_INLINE`) 时，字节直接内联存储在栈结构体内，**0 次堆分配** |
| `heap_ptr` | `[*]u8!` | 当 `len > 15` 时，自动回退至堆分配内存指针 |
| `length` | `u32` | 字符串精确字节长度 |
| `inlined` | `bool` | 标记当前处于栈内联模式 (`true`) 还是堆分配模式 (`false`) |

- 调用 `smol.isInline()`（或 `smol.isInlined()`）可查询内联状态；调用 `smol.release()` 仅在 `!inlined` 时释放堆内存，内联状态下为空操作。

### 2. `HashedString` — 预计算哈希与 $O(1)$ 快速拒绝比较

`HashedString` 在初始化时通过 `hashBytes32`（32 位 FNV-1a 算法，种子 `2166136261`，乘数 `16777619`）一次性计算出 `u32` 哈希指纹并缓存于结构体内：
- 在执行 `h1.eql(h2)` 时，首先对比 `h1.hash != h2.hash` 与 `h1.data.len != h2.data.len`，仅在哈希值与长度完全一致时才触发底层 `memcmp`，将符号表与路由匹配的绝大多数不相等比较开销降至 **$O(1)$ 单条寄存器比较指令**。

### 3. `Utf8Grapheme` & `Utf8`/`Utf16` — Unicode 编解码与 UAX #29 字素簇分析

- **UTF-8 / WTF-8 / UTF-16LE 互转 (`Utf8`, `Utf16`)**：
  - `Utf8.isValid` 先通过 `SimdScanner.firstNonAsciiIndex` 以每周期 16 字节跳过纯 ASCII 前缀，再按 RFC 3629 严格校验过长编码（Overlong）与孤立代理项（Surrogates `0xD800..0xDFFF`）。
  - `Utf16.encodeUtf8ToUtf16Le` 与 `decodeUtf16LeToUtf8` 完整处理非 BMP 平面字符（`U+10000..U+10FFFF`）与 UTF-16 高低代理对（High/Low Surrogate Pairs `0xD800..0xDBFF` + `0xDC00..0xDFFF`），遇到畸形序列自动回退为替换字符 `U+FFFD` (`REPLACEMENT_CHAR`)。
- **UAX #29 扩展字素簇计数 (`Utf8Grapheme.countGraphemes`)**：
  - 区分字节长度（`.len`）、Unicode 码点数（`countCodepoints`）与用户视觉感知的字素簇数（`countGraphemes`）。
  - 自动合并 `\r\n` (CRLF)、组合变音符号（`U+0300..U+036F`）、异体字选择器（`U+FE00..U+FE0F`）、肤色修饰符（`U+1F3FB..U+1F3FF`）以及零宽连接符序列（ZWJ `U+200D`，如家庭 Emoji `"👨‍👩‍👧"` 包含 18 个 UTF-8 字节、5 个 Unicode 码点，但 `countGraphemes` 精确返回 `1`）。
- **终端可视列宽计算 (`UnicodeWidth.visibleWidth`)**：
  - 自动剥离 ANSI CSI 颜色转义序列（`\x1b[...m`），并按东亚全角宽度规范（CJK 统一表意文字、全角标点、Emoji 计 2 列，ASCII 计 1 列，控制字符与组合音标计 0 列）计算终端对齐宽度。

---

## 三、 `cap:string` 与 Bun String 模块对照表

| 功能模块 | `cap:string` API | Bun 源码对照 (`bun/src/string/`) | 核心优势 |
| :--- | :--- | :--- | :--- |
| **短字符串优化** | `SmolStr.init(s)`, `slice()`, `isInline()` | `bun.SmolStr` (`SmolStr.cp`) | `<=15` 字节纯栈内联零堆分配 |
| **预哈希字符串** | `HashedString.init(s)`, `eql(other)` | `bun.HashedString` (`HashedString.cp`) | $O(1)$ 哈希先行比对快速拒绝 |
| **两阶段构建器** | `StringBuilder`: `count` -> `allocate` -> `append` | `bun.StringBuilder` (`StringBuilder.cp`) | 预统计总长，单次精确分配零扩容 |
| **分隔符连接器** | `StringJoiner.join(slices, delim)` / `pushDelimited` | `bun.StringJoiner` (`StringJoiner.cp`) | 批量切片一次分配完成连接 |
| **UTF-8/16 转码** | `Utf8.isValid`, `Utf16.encodeUtf8ToUtf16Le` | `bun.strings.convertUTF8toUTF16InBuffer` | 原生代理对编解码与 `0xFFFD` 容错 |
| **字素簇与列宽** | `Utf8Grapheme.countGraphemes`, `UnicodeWidth.visibleWidth` | `bun.strings.visibleWidth` / `uucode` | 支持 ZWJ Emoji 合并与 ANSI 颜色剥离 |
| **命名风格转换** | `CaseConverter.toSnakeCase` / `toCamelCase` / `toPascalCase` | `bun.strings` 标识符转换工具 | 智能切分连续大写缩写（如 `HTMLParser`） |
| **HTML/正则转义** | `StringEscape.escapeHtml` / `escapeRegExp` | `escapeHTML.cp` / `escapeRegExp.cp` | 零分配输出缓冲区转义 |

---

## 四、 代码示例

### 1. `SmolStr` (SSO) 与 `HashedString` ($O(1)$ 比较)

```typescript
import { SmolStr, HashedString } from "cap:string";

fn demoSmolAndHashed(): void {
    // 1. SmolStr: <= 15 字节完全在栈上内联
    let s1 = SmolStr.init("content-type");
    // s1.isInline() => true (零堆分配)
    defer s1.release();

    let s2 = SmolStr.init("x-custom-long-header-name");
    // s2.isInline() => false (自动回退堆分配)
    defer s2.release();

    // 2. HashedString: 预计算 FNV-1a 哈希
    const k1 = HashedString.init("authorization");
    const k2 = HashedString.init("authorization");
    const k3 = HashedString.init("accept-encoding");

    let match1 = k1.eql(k2); // true
    let match2 = k1.eql(k3); // false (通过 hash != other.hash 在 O(1) 内瞬间拒绝)
}
```

### 2. UTF-8 / UTF-16LE 转码、UAX #29 字素簇与终端显示列宽

```typescript
const std = @import("std");
import { Utf8, Utf16, Utf8Grapheme, UnicodeWidth } from "cap:string";

fn demoUnicodeAndGraphemes(): void {
    // 1. 复杂 ZWJ Emoji 字素簇分析
    const family_emoji = "👨‍👩‍👧";
    let byte_len = family_emoji.len;                          // 18 字节
    let cp_count = Utf8Grapheme.countCodepoints(family_emoji); // 5 个码点
    let gr_count = Utf8Grapheme.countGraphemes(family_emoji);  // 1 个视觉字素簇

    // 2. UTF-8 与 UTF-16LE 无损往返转码
    const text = "Hello, 世界! 🚀";
    let u16_buf: [32]u16 = [0; 32];
    const u16_len = Utf16.encodeUtf8ToUtf16Le(text, u16_buf[0..32]);

    let u8_buf: [64]u8 = [0; 64];
    const u8_len = Utf16.decodeUtf16LeToUtf8(u16_buf[0..u16_len], u8_buf[0..64]);
    // u8_buf[0..u8_len] == "Hello, 世界! 🚀"

    // 3. 剥离 ANSI 转义码的东亚全角终端列宽计算
    let cols = UnicodeWidth.visibleWidth("\x1b[32m你好CP\x1b[0m"); // 2*2 + 2 = 6 列
}
```

### 3. `CaseConverter` 命名风格转换与 `StringBuilder` 两阶段零重分配拼接

```typescript
import { CaseConverter, StringBuilder } from "cap:string";

fn demoCaseAndBuilder(): void {
    let out: [64]u8 = undefined;

    // 智能识别缩写词边界：HTMLParserEngine -> html_parser_engine
    let n1 = CaseConverter.toSnakeCase("HTMLParserEngine", &out);
    // out[0..n1] => "html_parser_engine"

    let n2 = CaseConverter.toCamelCase("max_retry_timeout", &out);
    // out[0..n2] => "maxRetryTimeout"

    // 两阶段零重分配 StringBuilder
    let sb = StringBuilder.init();
    defer sb.release();

    // Pass 1: 统计精确容量
    sb.count("https://");
    sb.count("api.cap.dev");
    sb.count("/v1/agents");
    sb.allocate(); // 仅执行唯一一次内存分配

    // Pass 2: 零重分配填充内容
    * = sb.append("https://");
    * = sb.append("api.cap.dev");
    * = sb.append("/v1/agents");
    // sb.slice() => "https://api.cap.dev/v1/agents"
}
```
