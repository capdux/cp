# `cap:byterange` — 零堆分配 HTTP Range 与 Content-Range 解析格式化库

`cap:byterange` 吸收自 Bun 上游静态文件服务器 `Range` 解析器（`src/runtime/server/RangeRequest.cp`），遵循 WebKit `parseRange`（WHATWG Fetch `#simple-range-header-value`）与 RFC 9110 §14，提供 100% 纯 CP、零堆分配（zero-alloc）的 HTTP `Range: bytes=...` 单段/多段解析与解析后延迟求值（`RawRange.resolve`），以及 `Content-Range` 响应头格式化与解析。

## 核心特性

- **100% 零堆分配**：`RawRange` 不持有任何输入缓冲区切片，可安全存储于请求上下文直至文件长度已知后再调用 `resolve(total)`。
- **完全兼容 Bun / WebKit 静态服务器语义**：
  - 大小写不敏感 `"bytes"` 单位、等号前允许空白；
  - 支持 `bytes=start-end`、`bytes=start-`（开放右端）、`bytes=-suffix`（后缀长度）；
  - `end >= total` 自动钳位至 `total - 1`；`start >= total` 或 `bytes=-0` 返回 `Unsatisfiable`（HTTP 416）；`end < start` 或非 `bytes` 单位安全回落 `None`（HTTP 200）。
- **RFC 9110 多段范围支持（`parseMultiRange`）**：支持逗号分隔的多段 `bytes=0-99, 200-299, -50` 零分配解析至调用方切片。
- **`Content-Range` 双向编解码**：支持 `formatContentRange`（`206 Partial Content`）、`formatUnsatisfiableContentRange`（`416` 的 `bytes */total`）与 `parseContentRange`（含未知总长 `bytes start-end/*`）。

---

## 导出清单

### 枚举与结构体

- `ByteRange`：`{ start: u64, end: u64 }`，附 `length(): u64`
- `RangeStatus`：`None | Satisfiable | Unsatisfiable`
- `RangeResult`：`{ status: RangeStatus, range: ByteRange }`
- `RawRangeKind`：`None | Suffix | Bounded`
- `RawRange`：`{ kind, start, end, suffix_len }`，附 `resolve(total: u64): RangeResult`
- `MultiRangeResult`：`{ status: RangeStatus, count: usize }`
- `ParsedContentRange`：`{ is_unsatisfied, start, end, total }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `parseRaw(header): RawRange` | 预解析单段 `Range: bytes=...`（独立于总长度） |
| `parseRange(header, total): RangeResult` | 一步解析并针对 `total` 字节求解单段范围 |
| `parseMultiRange(header, total, out_ranges): MultiRangeResult` | RFC 9110 多段 `Range` 零分配解析 |
| `formatContentRange(range, total, out_buf): usize?` | 格式化 `bytes <start>-<end>/<total>` |
| `formatUnsatisfiableContentRange(total, out_buf): usize?` | 格式化 `bytes */<total>` |
| `parseContentRange(header): ParsedContentRange?` | 解析 `Content-Range` 响应头 |

---

## 使用示例

```typescript
import std from "std";
import {
    RangeStatus,
    parseRange,
    formatContentRange,
    formatUnsatisfiableContentRange,
} from "cap:byterange";

fn rangeDemo() {
    const res = parseRange("bytes=100-199", 1000);
    let hdr_buf: [64]u8 = std.mem.zeroes([64]u8);
    let hdr_s: []u8 = hdr_buf[0..];
    if (res.status == RangeStatus.Satisfiable) {
        const n = formatContentRange(res.range, 1000, &hdr_s); // "bytes 100-199/1000"
        * = n;
    } else if (res.status == RangeStatus.Unsatisfiable) {
        const n = formatUnsatisfiableContentRange(1000, &hdr_s); // "bytes */1000"
        * = n;
    }
}
```
