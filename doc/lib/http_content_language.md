# cap:http_content_language

> 源码吸收自 `/data/dev/bun/src/http/`（RFC 9110 §8.5 + RFC 5646 HTTP `Content-Language` 响应头规范），提供纯 CP、零外部依赖、零堆分配的 BCP 47 语言标签校验、拆解、迭代、规范化大小写格式化与查询库。

## 导入方式

```cp
import {
    LanguageTagParts,
    isValidBcp47Tag,
    parseLanguageTagParts,
    ContentLanguageIter,
    canonicalizeLanguageTag,
    hasContentLanguage,
} from "cap:http_content_language";
```

## 核心类型与 API

- `LanguageTagParts`：结构体，包含 `primary: []const u8`（如 `"zh"`）、`script: ([]const u8)?`（4 字母如 `"Hans"`）、`region: ([]const u8)?`（2 字母或 3 数字如 `"CN"` / `"419"`）。
- `isValidBcp47Tag(tag: []const u8): bool`：校验基本 BCP 47 语法（首段 `2..=3` 字母，后续 `"-"` 分隔 `1..=8` 字母数字），拒绝空段与非法字符。
- `parseLanguageTagParts(tag: []const u8): LanguageTagParts?`：提取合法标签的 `primary`、`script` 与 `region`。
- `ContentLanguageIter`：
  - `init(header: []const u8): ContentLanguageIter`
  - `next(*_): ([]const u8)?`：零分配遍历逗号分隔的合法 BCP 47 标签，自动跳过空项、前后 OWS 与非法标签。
- `canonicalizeLanguageTag(tag: []const u8, out: []u8): ([]const u8)?`：按 RFC 5646 §2.1.1 规范化大小写（`zh-hans-cn` -> `zh-Hans-CN`、`EN-us` -> `en-US`、`en-x-US` -> `en-x-us`），非法标签或输出缓冲不足返回 `null`。
- `hasContentLanguage(header: []const u8, target: []const u8): bool`：大小写不敏感匹配完整语言标签或主语种前缀。

## 使用示例

```cp
import {
    parseLanguageTagParts,
    canonicalizeLanguageTag,
    hasContentLanguage,
} from "cap:http_content_language";

fn example() {
    * = hasContentLanguage("zh-Hans-CN, en-US", "zh"); // true
    * = parseLanguageTagParts("es-419"); // primary="es", script=null, region="419"
    let buf: [32]u8 = undefined;
    * = canonicalizeLanguageTag("zh-hans-cn", &buf); // "zh-Hans-CN"
}
```
