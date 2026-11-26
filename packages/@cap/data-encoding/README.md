# @cap/data-encoding

Zero-allocation RFC 4648 Base32, Base32hex, Crockford Base32, and Base64url (no-pad) encoder/decoder for CP — clean-room MIT parity with Rust `data-encoding`.

零堆分配的 CP 多基数安全编解码引擎，统一实现 RFC 4648 Base32（含填充/无填充）、Base32hex（保持二进制字典序）、Crockford Base32（自动容错 `i/l→1`, `o→0` 与忽略 `-`）及无填充 Base64url，严格校验字符集与非零尾位（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/data-encoding
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `encodeBase32` / `decodeBase32` | `(input, pad, out)` / `(encoded, out) -> string?` | RFC 4648 Base32（`A-Z2-7`），严格校验非零尾位 |
| `encodeBase32Hex` / `decodeBase32Hex` | `(input, pad, out)` / `(encoded, out) -> string?` | RFC 4648 Base32hex（`0-9A-V`） |
| `encodeCrockford` / `decodeCrockford` | `(input, out)` / `(encoded, out) -> string?` | Crockford Base32（`0-9A-HJKMNP-TV-Z`，解码宽容 `i/l/o` 与 `-`） |
| `encodeBase64UrlNoPad` / `decodeBase64UrlNoPad` | `(input, out)` / `(encoded, out) -> string?` | RFC 4648 Base64url 无填充编解码（JWT/URL 安全） |

## Usage / 使用示例

```cp
import { encodeBase32, decodeBase32, encodeBase64UrlNoPad } from "@cap/data-encoding";

let buf: [64]u8 = undefined;
const b32 = encodeBase32("foobar", true, buf[0..64]); // "MZXW6YTBOI======"
const b64 = encodeBase64UrlNoPad("Hello?", buf[0..64]);
```

## License

MIT
