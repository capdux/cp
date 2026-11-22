# `cap:textstream` — 零堆分配 WHATWG 流式 UTF-8 编码器（TextEncoderStream）

> 参考来源：/data/dev/bun/src/runtime/webcore/TextEncoderStreamEncoder.cp | 吸收日期：2026-11-22

`cap:textstream` 吸收自 Bun 上游 WebCore `TextEncoderStream` 实现（`src/runtime/webcore/TextEncoderStreamEncoder.cp`），剥离了 JSC 与堆分配器耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 WHATWG Encoding Standard 流式 UTF-8 编码状态机，支持跨分块 UTF-16 高低代理对无缝拼接、孤立代理替换（`U+FFFD`）、Latin-1 流式转码及精确输出字节长度预计算。

## 核心特性

- **100% 零堆分配**：`TextEncoderStream` 仅含单个 `pending_lead_surrogate: u16?` 状态字段，所有编码输出均写入调用方切片，缓冲不足时原子返回 `null` 不破坏内部状态。
- **跨分块 UTF-16 代理对拼接**：当分块末尾遇到高代理（`0xD800..0xDBFF`）时自动暂存；若下一块首码元为低代理（`0xDC00..0xDFFF`），合成补充平面码点输出 4 字节 UTF-8（如 Emoji `😀`、`🚀`）。
- **严格孤立代理替换（`U+FFFD`）**：高代理后跟非低代理、流首孤立低代理、跨接 Latin-1 分块或流结束 `flush()` 时，均按 WHATWG 规范输出 3 字节 `0xEF 0xBF 0xBD`。
- **精确长度预计算**：内置 `utf8ByteLengthFromLatin1` 与 `utf8ByteLengthFromUtf16`，方便调用方精确规划零分配输出缓冲。

---

## 导出清单

### 常量与原语

- `UTF8_REPLACEMENT: [3]u8 = [0xEF, 0xBF, 0xBD]`
- `isLeadSurrogate(cu: u16): bool` / `isTrailSurrogate(cu: u16): bool`
- `combineSurrogatePair(lead: u16, trail: u16): u32?`
- `encodeCodePointUtf8(cp: u32, out_buf: *[]u8): usize?`
- `utf8ByteLengthFromLatin1(input: []const u8): usize`
- `utf8ByteLengthFromUtf16(input: []const u16, has_pending_lead: bool): usize`

### 核心状态机 `TextEncoderStream`

| 方法签名 | 说明 |
| :--- | :--- |
| `TextEncoderStream.init(): TextEncoderStream` | 创建初始无挂起代理的流式编码器 |
| `hasPendingSurrogate(): bool` / `reset(*_)` | 查询或重置挂起的高代理状态 |
| `encodeLatin1(*_, input: []const u8, out_buf: *[]u8): usize?` | 流式编码 Latin-1 分块至 UTF-8 |
| `encodeUtf16(*_, input: []const u16, out_buf: *[]u8): usize?` | 流式编码 UTF-16 分块至 UTF-8（跨块代理拼接） |
| `flush(*_, out_buf: *[]u8): usize?` | 结束流并刷新残留高代理为 `U+FFFD` |

---

## 使用示例

```typescript
import std from "std";
import { TextEncoderStream } from "cap:textstream";

fn textStreamDemo() {
    let enc = TextEncoderStream.init();
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    let out: []u8 = buf[0..];

    // Split U+1F600 (😀 = 0xD83D, 0xDE00) across two UTF-16 chunks
    const chunk1: [2]u16 = ['A', 0xD83D];
    const chunk2: [2]u16 = [0xDE00, '!'];

    const n1 = enc.encodeUtf16(chunk1[0..], &out) ?? return; // "A" (1 byte), 0xD83D buffered
    const n2 = enc.encodeUtf16(chunk2[0..], &out) ?? return; // "😀!" (5 bytes)
    const nf = enc.flush(&out) ?? return;                    // 0 bytes
    * = n1;
    * = n2;
    * = nf;
}
```

## 上游行为契约与流式编码边界

1. **原子缓冲不足防御**：
   - `encodeLatin1` / `encodeUtf16` / `flush` 在 `out_buf` 容量不足时返回 `null` 且不破坏 `pending_lead_surrogate` 内部状态；调用方可使用 `utf8ByteLengthFromLatin1` / `utf8ByteLengthFromUtf16(input, enc.hasPendingSurrogate())` 提前精确计算所需字节数。
2. **流结束必须调用 `flush()`**：
   - 若最后一个 UTF-16 分块以高代理（`0xD800..0xDBFF`）结尾，编码器会将其暂存以等待可能的后续低代理；流终止时必须调用 `flush(&out)` 将孤立高代理冲刷为 3 字节 `U+FFFD`（`UTF8_REPLACEMENT`）。
