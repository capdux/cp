<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:whatwg-mimetype` ([https://github.com/jsdom/whatwg-mimetype](https://github.com/jsdom/whatwg-mimetype)) | 许可：`MIT`

# `@cap/whatwg-mimetype`

零堆分配纯 CP WHATWG MIME Sniffing 标准解析器与序列化器，对标 npm **#200** `whatwg-mimetype`（真实周下载 **2.15 亿/周**）。

## 核心 API

- `parseMimeType(input: string): MimeType` — 遵循 WHATWG §4.4 解析主类型、子类型与参数（首次出现优先、引号剥离）
- `MimeType.essence(out)` / `.serialize(out)` / `.getParam(name)`
- `MimeType.isHTML()` / `.isXML()` / `.isJavaScript()` / `.isJSON()`
