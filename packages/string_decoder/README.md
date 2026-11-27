[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string_decoder` ([https://github.com/nodejs/string_decoder](https://github.com/nodejs/string_decoder)) | 许可：`MIT`

# `@cap/string-decoder`

零堆分配纯 CP 流式多字节字符串解码器，完整对标 Node.js 官方 `string_decoder`（npm **#62**，真实周下载 **3.78 亿/周**），跨分片精确缓冲被截断的 UTF-8 2/3/4 字节字符与 UTF-16LE 代理对，在 `.end()` 时对残缺尾字节输出标准 `U+FFFD`。

## 核心 API

- `StringDecoder.utf8()` / `StringDecoder.new(DecoderEncoding)`
- `.write(chunk: []const u8, out: []u8): usize`
- `.end(out: []u8): usize`
- `.pendingBytes(): usize`
