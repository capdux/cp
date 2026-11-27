[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/picohttp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 @cap/picohttp

## 安装与导入

```bash
cap add @cap/picohttp
```

```cp
import { phr_parse_request } from "@cap/picohttp";
```

## 核心导出 API

- `export fn phr_parse_request(buf: [*]&u8!, len: usize, method: *[*]&u8!?!, method_len: *usize!, path: *[*]&u8!?!, path_len: *usize!, minor_version: *i32!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;`
- `export fn phr_parse_response(buf: [*]&u8!, len: usize, minor_version: *i32!, status: *i32!, msg: *[*]&u8!?!, msg_len: *usize!, headers: [*]phr_header!, num_headers: *usize!, last_len: usize): i32;`
- `export fn phr_decode_chunked(decoder: *phr_chunked_decoder!, buf: [*]u8!, bufsz: *usize!): isize;`
- `export fn phr_decode_chunked_is_in_data(decoder: *phr_chunked_decoder!): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`picohttpparser@1.0.0`)，许可证：MIT
