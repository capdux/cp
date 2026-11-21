# `cap:md_links` — CommonMark §6.3–6.5 内联链接、图片与自动链接解析器

> 溯源：从 `/data/dev/bun/src/md/links.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:md_links` 提供零外部依赖、零堆分配的 CommonMark §6.3–6.5 链接、图片与自动链接解析器及 URL 百分号编码工具：

- 支持内联链接 `[text](url "title")`、图片 `![alt](src)`（含嵌套图片链接 `[![alt](img)](url)`、转义括号与代码跨度跳过）、平衡圆括号 URL、`<...>` 尖括号目标及 `"..."`/`'...'`/`(...)` 三种标题；
- 支持 full (`[text][ref]`)、collapsed (`[text][]`) 与 shortcut (`[text]`) 三种引用链接；
- 支持 CommonMark §6.5 URI（2..32 字节 scheme）与 HTML5 Email 尖括号自动链接；
- 内置零分配 `percentEncodeUrl`，安全编码空格/非 ASCII/控制字符同时保留已合法的 `%XX` 序列。

---

## 快速导入

```cp
import {
    LinkKind,
    ParsedLink,
    parseAutolink,
    parseInlineLinkOrImage,
    percentEncodeUrl,
} from "cap:md_links";
```

---

## API 参考

| 符号 / 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `LinkKind` | `inline_link`, `image`, `full_ref`, `collapsed_ref`, `shortcut_ref`, `autolink_uri`, `autolink_email` | 链接类型枚举（提供 `isImage()`, `isAutolink()`, `isReference()`） |
| `ParsedLink` | `{ kind, text, dest, title, start_pos, end_pos }` | 零拷贝解析结果（切片直接指向原始文本） |
| `parseAutolink` | `pub fn parseAutolink(text: []const u8, pos: usize): ParsedLink?` | 解析 `<scheme:...>` 与 `<user@domain>` 自动链接 |
| `parseInlineLinkOrImage` | `pub fn parseInlineLinkOrImage(text: []const u8, start_pos: usize): ParsedLink?` | 解析内联链接、图片及三种引用链接 |
| `percentEncodeUrl` | `pub fn percentEncodeUrl(raw: []const u8, out: []u8): ([]const u8)?` | 安全百分号编码（缓冲不足返回 `null`） |

---

## 使用示例

```cp
import { parseInlineLinkOrImage, parseAutolink, percentEncodeUrl } from "cap:md_links";

fn demo() {
    if (link := parseInlineLinkOrImage("see [CP](https://cp-lang.org \"Home\")", 4)) {
        * = link.dest;  // "https://cp-lang.org"
        * = link.title; // "Home"
    }
    if (auto := parseAutolink("<dev@cp-lang.org>", 0)) {
        * = auto.kind; // .autolink_email
    }
    let buf: [64]u8 = [0; 64];
    if (enc := percentEncodeUrl("https://ex.com/a b%20c", buf[0..64])) {
        * = enc; // "https://ex.com/a%20b%20c"
    }
}
```
