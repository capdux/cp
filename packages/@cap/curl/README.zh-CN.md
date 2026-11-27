[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/curl

> 版本：`8.23.0` | 许可证：`curl` | 类型声明：[src/index.d.cp](./src/index.d.cp)

libcurl (v8.23.0) multiprotocol network transfer library (HTTP/FTP/Proxy/Multipart) for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/curl
```

```cp
import { cp_curl_version } from "@cap/curl";
```

## 核心导出 API

- `export fn cp_curl_version(): [*:0]&u8!;`
- `export fn cp_curl_global_init(): i32;`
- `export fn cp_curl_global_cleanup();`
- `export fn cp_curl_http_get(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?;`
- `export fn cp_curl_free(ptr: [*]u8!);`
- `export fn version(): string;`
- `export fn globalInit(): !void;`
- `export fn globalCleanup();`
- `export fn httpGet(url: string): !CurlResponse;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`curl@8.23.0`)，许可证：curl
