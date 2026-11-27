# @cap/data-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:data-uri-to-buffer` ([https://github.com/TooTallNate/proxy-agents](https://github.com/TooTallNate/proxy-agents))（原库名 `data-uri-to-buffer` → CP 包名 `@cap/data-uri`）

Zero-allocation RFC 2397 `data:[<mediatype>][;base64],<data>` URI parser, Base64/percent decoder, and builder for CP — clean-room MIT parity with npm `data-uri-to-buffer` / `datauri` and Rust `data-url`.

零堆分配的 CP RFC 2397 Data URI 解析、Base64/百分号解码与编码构建引擎，支持默认 `text/plain;charset=US-ASCII`、大小写无关 `data:` 与 `;base64`、UTF-8 百分号解码以及二进制载荷无损 Base64 往返（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/data-uri
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `DataUri` | `pub struct { ok: bool, media_type: string, charset: string, is_base64: bool, raw_payload: string }` | 解析结果结构体（零拷贝指向原 URI 切片） |
| `parseDataUri` | `(uri: string) -> DataUri` | 解析 RFC 2397 Data URI（缺省 `media_type="text/plain"`, `charset="US-ASCII"`） |
| `decodeDataUri` | `(uri: string, out: []u8) -> ([]u8)?` | 自动按 `is_base64` 执行 Base64 或 `%XX` 百分号解码，非法输入返回 `null` |
| `encodeDataUriBase64` | `(media_type: string, payload: []u8, out: []u8) -> string` | 构建 `data:<media_type>;base64,<b64>` |
| `encodeDataUriText` | `(media_type: string, text: string, out: []u8) -> string` | 构建百分号编码的 `data:<media_type>,<encoded>` |
| `isDataUri` | `(s: string) -> bool` | 快速判断字符串是否为合法 RFC 2397 Data URI |

## Usage / 使用示例

```cp
import {
    parseDataUri,
    decodeDataUri,
    encodeDataUriBase64,
    encodeDataUriText,
    isDataUri,
} from "@cap/data-uri";

let buf: [256]u8 = undefined;
const uri = "data:text/plain;charset=utf-8;base64,SGVsbG8sIFdvcmxkIQ==";
const info = parseDataUri(uri);
// info.ok == true, info.media_type == "text/plain", info.charset == "utf-8", info.is_base64 == true

const decoded = decodeDataUri(uri, buf[0..256]).?;
// decoded == "Hello, World!"
```

## License

MIT
