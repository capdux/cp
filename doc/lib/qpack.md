# `cap:qpack` — 零堆分配 HTTP/3 QPACK 静态表与 Field Section 编解码库

> 参考来源：`/data/dev/bun/src/uws_sys/quic/Header.cp`, `/data/dev/bun/src/http/h3_client/encode.cp` | 吸收日期：2026-11-22

`cap:qpack` 吸收自 Bun 上游 HTTP/3 QUIC 头部子系统（`src/uws_sys/quic/Header.cp` 与 `src/http/h3_client/encode.cp`），剥离了底层 uWS/QUIC 宿主绑定，提供 100% 纯 CP、零堆分配（zero-alloc）的 RFC 9204 QPACK 静态表索引分类、变长前缀整数编解码、HTTP/3 出站请求伪头部规范编排以及静态 QPACK Field Section 零拷贝编解码。

## 核心特性

- **100% 零堆分配**：头部列表、小写名称缓冲与 Field Section 字节流全部由调用方以切片传入，解码采用零拷贝切片视图。
- **RFC 9204 静态表分类（`classifyHeader` / `staticIndexToName`）**：大小写不敏感匹配 Bun HTTP/3 客户端全部禁用 hop-by-hop 头（`connection`、`keep-alive`、`proxy-connection`、`transfer-encoding`、`upgrade`）、`host` 以及 32 项静态表索引头。
- **HTTP/3 出站请求头部编排（`buildHttp3RequestHeaders`）**：按 Bun `h3_client/encode.cp` 约定将 `:method`/`:scheme`/`:authority`/`:path` 置于槽位 `0..3`（支持 `Host` 覆盖 `:authority`、空路径回落 `"/"`），自动剔除禁用头、命中静态索引零拷贝复用规范小写名、未知自定义头零分配小写化。
- **RFC 9204 §4.1.1 前缀整数编解码**：支持 `1..8` 位前缀的无符号变长整数编码与防截断/防溢出解码。
- **无动态表 Field Section 往返编解码（`encodeFieldSection` / `decodeFieldSection`）**：严格遵循 RFC 9204 §4.5（`RIC = 0, Delta Base = 0`），自动为敏感头（`authorization`/`cookie`/`set-cookie`）置 `N = 1`（never-indexed）位。

---

## 导出清单

### 常量、枚举与结构体

- `QPACK_NO_INDEX: i32 = -1` 及 `QPACK_IDX_*` 静态表索引常量（`0..96`）
- `QpackHeader`：`{ name, value, qpack_index }`，附 `QpackHeader.init(name, value, qpack_index)`
- `QpackClassKind`：`Forbidden | Host | Indexed | Literal`
- `QpackClass`：`{ kind, canonical_name, static_index }`
- `IntDecodeResult`：`{ value, bytes_read }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `classifyHeader(name): QpackClass` | 大小写不敏感分类 HTTP/3 头部与静态索引 |
| `staticIndexToName(index): []const u8?` | 从 RFC 9204 静态表索引反查规范小写头部名 |
| `encodePrefixedInt(prefix_bits, prefix_mask, value, out_buf): usize?` | RFC 9204 N-bit 前缀整数编码 |
| `decodePrefixedInt(prefix_bits, data): IntDecodeResult?` | RFC 9204 N-bit 前缀整数解码（含溢出/截断防护） |
| `buildHttp3RequestHeaders(method, scheme, default_authority, path, raw_headers, out_headers, lower_buf): usize?` | HTTP/3 出站请求头部规范编排 |
| `encodeFieldSection(headers, out_buf): usize?` | 静态 QPACK Field Section 编码 |
| `decodeFieldSection(data, out_headers): usize?` | 静态 QPACK Field Section 零拷贝解码 |

---

## 使用示例

```typescript
import std from "std";
import {
    QPACK_NO_INDEX,
    QpackHeader,
    buildHttp3RequestHeaders,
    encodeFieldSection,
    decodeFieldSection,
} from "cap:qpack";

fn qpackDemo() {
    const raw = [
        QpackHeader.init("Host", "api.example.com", QPACK_NO_INDEX),
        QpackHeader.init("Connection", "keep-alive", QPACK_NO_INDEX),
        QpackHeader.init("User-Agent", "cp-h3/1.0", QPACK_NO_INDEX),
        QpackHeader.init("X-Trace-Id", "abc123", QPACK_NO_INDEX),
    ];
    let h3_hdrs: [8]QpackHeader = std.mem.zeroes([8]QpackHeader);
    let lower_arr: [64]u8 = std.mem.zeroes([64]u8);
    let h3_s: []QpackHeader = h3_hdrs[0..];
    let lower_s: []u8 = lower_arr[0..];

    const n = buildHttp3RequestHeaders("GET", "https", "fallback.local", "/v1/ping", raw[0..], &h3_s, &lower_s) ?? return;

    let wire: [256]u8 = std.mem.zeroes([256]u8);
    let wire_s: []u8 = wire[0..];
    const wlen = encodeFieldSection(h3_hdrs[0..n], &wire_s) ?? return;

    let decoded: [8]QpackHeader = std.mem.zeroes([8]QpackHeader);
    let dec_s: []QpackHeader = decoded[0..];
    const dcnt = decodeFieldSection(wire[0..wlen], &dec_s);
    * = dcnt;
}
```

---

## 上游 Bun HTTP/3 QPACK 行为契约与安全边界

1. **伪头部槽位顺序与 `Host` 合并（`buildHttp3RequestHeaders`）**：
   - RFC 9114 要求所有伪头部（`:method`、`:scheme`、`:authority`、`:path`）必须位于普通头部之前；当调用方传入 `Host` 头时自动覆盖 `:authority` 并剔除冗余 `Host`，空路径自动回落为 `"/"`。
2. **零动态表无队头阻塞（`RIC = 0`）**：
   - `encodeFieldSection` / `decodeFieldSection` 采用纯静态表 + 字面量编码（`Required Insert Count = 0`），彻底规避 QUIC 多流乱序到达时的 QPACK 动态表队头阻塞；敏感头自动置 `N = 1` 禁止中间节点索引。
3. **前缀整数溢出与截断防护（`decodePrefixedInt`）**：
   - 严格校验 `prefix_bits ∈ [1, 8]` 与无符号位移溢出边界，防止畸形 QUIC 头部帧引发无限续位或整数回绕。
