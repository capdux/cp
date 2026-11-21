# `cap:yenc` — yEnc v1.3 二进制转文本编解码与头尾解析库

纯 CP、零堆分配的 yEnc v1.3 编解码与报文头尾解析库，完整覆盖临界字节转义（`NUL`/`\n`/`\r`/`=`、行首 `.`、行首尾空格/TAB）、自动 `\r\n` 折行、单/多段 `=ybegin`/`=ypart`/`=yend` 零拷贝解析以及 IEEE 802.3 CRC32 完整性校验。

## 导入

```cp
import {
    YENC_OFFSET,
    YENC_ESCAPE_OFFSET,
    YENC_ESCAPE_CHAR,
    YENC_DEFAULT_LINE_LEN,
    yencCrc32,
    yencEncode,
    yencDecode,
    YencBeginHeader,
    YencPartHeader,
    YencEndTrailer,
    parseYencBegin,
    parseYencPart,
    parseYencEnd,
} from "cap:yenc";
```

## 核心 API

- **编解码与校验**：
  - `yencCrc32(data: []const u8): u32` — IEEE 802.3 CRC32 (`0xEDB88320`)。
  - `yencEncode(src: []const u8, line_len: usize, dst: []u8): usize?` — `line_len == 0` 默认 `128`，自动处理临界转义与 `\r\n` 折行。
  - `yencDecode(src: []const u8, dst: []u8): usize?` — 跳过 `\r`/`\n`，反转义 `'='` 序列，严格拒绝孤立尾 `'='` 与输出缓冲溢出。
- **头尾行解析**：
  - `parseYencBegin(line: []const u8): YencBeginHeader?` — 支持单段/多段（`isMultipart()`）及行尾含空格 `name=...`。
  - `parseYencPart(line: []const u8): YencPartHeader?` — 1-based 闭区间 `begin`..`end`（`partSize()`）。
  - `parseYencEnd(line: []const u8): YencEndTrailer?` — 解析 `size`、可选 `part`、十六进制 `crc32`/`pcrc32`。

## 示例

```cp
import { yencEncode, yencDecode, yencCrc32 } from "cap:yenc";

fn verifyRoundtrip(raw: []const u8, enc: []u8, dec_buf: []u8): bool {
    if (elen := yencEncode(raw, 128, enc)) {
        if (dlen := yencDecode(enc[0..elen], dec_buf)) {
            return dlen == raw.len && yencCrc32(dec_buf[0..dlen]) == yencCrc32(raw);
        }
    }
    return false;
}
```
