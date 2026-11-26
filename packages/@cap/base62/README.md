# `@cap/base62`

Zero-heap URL-safe Base62 (`0-9A-Za-z`) encoder and decoder in pure CP, supporting both `u64` (variable & fixed-width lexicographically sortable padded IDs) and arbitrary byte slices with leading-zero preservation.

零堆分配纯 CP URL 安全 Base62（`0-9A-Za-z`）编解码引擎，对标 Rust/npm `base62`，支持 `u64` 短链/雪花 ID 变长与定宽保序编码，以及任意字节切片（严格保留前导零）双向转换。

## Installation

```bash
cap add @cap/base62
```

## API

| Function | Description |
| --- | --- |
| `encodeU64(value, dst)` | Encode `u64` into compact Base62 (`1..11` chars) |
| `encodeU64Padded(value, width, dst)` | Encode `u64` left-padded with `'0'` for lexicographic sorting |
| `decodeU64(src)` | Strict Base62 -> `u64` decode with overflow protection |
| `encodeBytes(src, dst)` | Encode arbitrary byte slice (<= 128 B) preserving leading zeros |
| `decodeBytes(src, dst)` | Decode Base62 string back to bytes |
| `isValidBase62(src)` | Validate Base62 alphabet membership |

## License

MIT
