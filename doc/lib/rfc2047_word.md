# cap:rfc2047_word

> 源码吸收自 `/data/dev/bun/src/http/MimeType.zig`（RFC 2047 MIME Encoded-Word 规范），提供纯 CP、零外部依赖、零堆分配的 `=?charset?B|Q?text?=` 编解码与头部值展开库。

## 导入方式

```cp
import {
    EncodedWordEncoding,
    EncodedWord,
    parseEncodedWord,
    decodeEncodedWord,
    decodeHeaderValue,
    encodeUtf8Base64Word,
} from "cap:rfc2047_word";
```

## 核心类型与 API

- `EncodedWordEncoding`：`base64` / `quoted_printable`。
- `EncodedWord`：`charset`、`encoding`、`encoded_text`。
- `parseEncodedWord(token): EncodedWord?`：严格解析 `=?charset?B|b|Q|q?text?=`，拒绝空白或空 charset。
- `decodeEncodedWord(token, out): ([]const u8)?`：解码 `B`（Base64）与 `Q`（`_`→空格、`=XX` 十六进制）。
- `decodeHeaderValue(header, out): ([]const u8)?`：混合文本解码，按 RFC 2047 §6.2 自动折叠相邻 encoded-word 间空白，非合法词原样保留。
- `encodeUtf8Base64Word(text, out): ([]const u8)?`：编码为 `=?UTF-8?B?<base64>?=`。

## 使用示例

```cp
import { encodeUtf8Base64Word, decodeHeaderValue } from "cap:rfc2047_word";

fn example() {
    let enc: [128]u8 = undefined;
    let dec: [128]u8 = undefined;
    if (w := encodeUtf8Base64Word("你好世界", &enc)) {
        * = decodeHeaderValue(w, &dec); // "你好世界"
    }
}
```
