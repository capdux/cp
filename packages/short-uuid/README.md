# @cap/short-uuid

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:short-uuid` ([https://github.com/oculus42/short-uuid](https://github.com/oculus42/short-uuid))

Zero-allocation 128-bit UUID to 22-character Flickr Base58 / URL Base62 / Cookie Base90 lossless short ID translator for CP — clean-room MIT parity with npm `short-uuid` and `uuid-translator`.

零堆分配的 CP 128-bit UUID 无损双向短 ID 压缩与还原引擎，支持将 36 字符标准 UUID（或 32 字符紧凑十六进制 UUID）无损压缩为定长 22 字符 `FLICKR_BASE58` / `URL_BASE62` 或 20 字符 `COOKIE_BASE90` URL 安全短 ID，100% 保证全 0 至全 F 极值双向无损往返（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/short-uuid
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `FLICKR_BASE58` | `pub const string` | 58 字符 Flickr 字母表（剔除易混淆 `0`/`O`/`I`/`l`），编码定长 22 字符 |
| `URL_BASE62` | `pub const string` | 62 字符 URL 安全字母表（`0-9A-Za-z`），编码定长 22 字符 |
| `COOKIE_BASE90` | `pub const string` | 90 字符 RFC 6265 Cookie 安全字母表，编码定长 20 字符 |
| `uuidToBytes` | `(uuid: string, out16: []u8) -> bool` | 解析 36 位带连字符或 32 位紧凑 UUID（大小写无关）为 16 字节 |
| `bytesToUuid` | `(bytes16: []u8, out36: []u8) -> string` | 将 16 字节格式化为标准小写 `8-4-4-4-12` UUID |
| `shortenUuid` | `(uuid: string, out: []u8) -> string?` | 使用 `FLICKR_BASE58` 压缩为定长 22 字符短 ID |
| `expandUuid` | `(short_id: string, out36: []u8) -> string?` | 将 `FLICKR_BASE58` 短 ID 无损还原为 36 字符小写 UUID |
| `shortenUuidWith` | `(uuid: string, alphabet: string, out: []u8) -> string?` | 使用自定义字母表无损压缩 UUID |
| `expandUuidWith` | `(short_id: string, alphabet: string, out36: []u8) -> string?` | 使用自定义字母表无损还原 UUID（检测非法字符与 128-bit 溢出） |

## Usage / 使用示例

```cp
import {
    URL_BASE62,
    shortenUuid,
    expandUuid,
    shortenUuidWith,
    expandUuidWith,
} from "@cap/short-uuid";

let short_buf: [32]u8 = undefined;
let uuid_buf: [36]u8 = undefined;

const orig = "550e8400-e29b-41d4-a716-446655440000";
const short_id = shortenUuid(orig, short_buf[0..32]).?; // 22-char Flickr Base58
const restored = expandUuid(short_id, uuid_buf[0..36]).?; // == orig

const b62 = shortenUuidWith(orig, URL_BASE62, short_buf[0..32]).?;
const restored62 = expandUuidWith(b62, URL_BASE62, uuid_buf[0..36]).?; // == orig
```

## License

MIT
