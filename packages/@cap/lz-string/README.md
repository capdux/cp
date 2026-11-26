# @cap/lz-string

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lz-string` ([https://github.com/pieroxy/lz-string](https://github.com/pieroxy/lz-string))（原库名 `lz-string` → CP 包名 `@cap/lz-string`）

零堆分配的 CP 原生 LZW 字节/字符串压缩库，Clean-Room MIT 实现对标 npm 顶流压缩包 `lz-string`（1.14 亿+/周下载）。

内置 4096 项定容静态字典（9~12 位变长编码 + 自动字典清空重置 + EOF 哨兵校验），支持原始字节流、标准 Base64 与 URL 安全编码（`A-Za-z0-9+-$`）的双向无损编解码。

## 核心 API

- `compressBytes(input: []const u8, out: []u8): usize` — 使用 4096 项静态字典对字节切片执行 LZW 压缩，返回写入长度
- `decompressBytes(compressed: []const u8, out: []u8): i64` — 解压 LZW 字节流，返回解压后长度；非法/截断输入或缓冲不足返回 `-1`
- `compressToBase64(input: string, out: []u8): usize` — 压缩并编码为标准 Base64 字符串
- `decompressFromBase64(b64: string, out: []u8): i64` — 从标准 Base64 字符串解码并解压
- `compressToEncodedURIComponent(input: string, out: []u8): usize` — 压缩并编码为 URL 安全字母表（`A-Za-z0-9+-$`）
- `decompressFromEncodedURIComponent(uri: string, out: []u8): i64` — 从 URL 安全编码解码并解压

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`pieroxy/lz-string`](https://github.com/pieroxy/lz-string)。
