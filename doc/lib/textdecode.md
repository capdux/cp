# `cap:textdecode` — 零堆分配 WHATWG 流式文本解码器（TextDecoder）

`cap:textdecode` 吸收自 Bun 上游 WebCore `TextDecoder` 实现（`src/runtime/webcore/TextDecoder.cp`），剥离了 JSC 与堆分配耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 WHATWG Encoding Standard 流式文本解码器，支持 **UTF-8**、**UTF-16LE**、**UTF-16BE** 与 **Latin-1** 跨分块流式解码至规范 UTF-8，完整覆盖 BOM 嗅探/剥离、`fatal` 严格模式、过长/代理非法序列拦截与 `U+FFFD` 替换。

## 核心特性

- **100% 零堆分配**：`TextDecoder` 为纯值类型流式状态机，跨块状态全部内联于结构体中，解码输出直接写入调用方 `[]u8` 缓冲。
- **WHATWG §8.1.1 严格 UTF-8 解码**：精确校验首字节与次字节合法下/上界（拦截过长编码如 `C0 AF`/`E0 80 AF`、UTF-16 代理码点 `ED A0 80` 及超范围 `F4 90 80 80`），支持任意单字节切分的跨分块续字节重组。
- **UTF-16LE / UTF-16BE 流式解码**：自动暂存跨块奇数字节（`lead_byte`）与跨块高代理（`lead_surrogate`），无缝还原补充平面字符并替换孤立代理。
- **BOM 嗅探与可选剥离**：内置 `sniffBom` 及 `ignore_bom` 开关，默认自动剥离首个 `U+FEFF` BOM（`EF BB BF` / `FF FE` / `FE FF`），`ignore_bom = true` 时原样保留。

---

## 导出清单

### 枚举与结构体

- `TextEncoding`：`Utf8 | Utf16Le | Utf16Be | Latin1`
- `DecodeResult`：`{ bytes_written: usize, had_error: bool }`

### 核心函数与状态机

| 签名 | 说明 |
| :--- | :--- |
| `sniffBom(data: []const u8): TextEncoding?` | 嗅探 UTF-8 / UTF-16LE / UTF-16BE BOM |
| `TextDecoder.init(encoding, fatal, ignore_bom): TextDecoder` | 创建流式文本解码器 |
| `TextDecoder.decode(*_, input, stream, out_utf8): DecodeResult?` | 流式解码字节块至规范 UTF-8（`stream=false` 触发 EOF 刷新） |
| `TextDecoder.hasPendingBytes(): bool` / `reset(*_)` | 查询未决跨块字节或重置状态机 |

---

## 使用示例

```typescript
import std from "std";
import { TextEncoding, TextDecoder, sniffBom } from "cap:textdecode";

fn textDecodeDemo() {
    const raw: [7]u8 = [0xEF, 0xBB, 0xBF, 0xF0, 0x9F, 0x98, 0x80]; // BOM + 😀
    const enc = sniffBom(raw[0..]) ?? TextEncoding.Utf8;
    let td = TextDecoder.init(enc, false, false);

    let out_arr: [16]u8 = std.mem.zeroes([16]u8);
    let out: []u8 = out_arr[0..];
    // Feed byte-by-byte across chunks
    const r1 = td.decode(raw[0..5], true, &out);
    const r2 = td.decode(raw[5..7], false, &out);
    * = r1;
    * = r2;
}
```
