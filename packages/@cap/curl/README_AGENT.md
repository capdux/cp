# @cap/curl@8.23.0 (Agent Quickref)

> libcurl (v8.23.0) multiprotocol network transfer library (HTTP/FTP/Proxy/Multipart) for CP via prebuilt dynamic library | License: curl
> Import: `import { ... } from "@cap/curl";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_curl_version(): [*:0]&u8!;`
- `export fn cp_curl_global_init(): i32;`
- `export fn cp_curl_global_cleanup();`
- `export fn cp_curl_http_get(url: [*:0]&u8!, out_status: &mut i64, out_len: &mut usize): [*]u8!?;`
- `export fn cp_curl_free(ptr: [*]u8!);`
- `export fn version(): string;`
- `export fn globalInit(): !void;`
- `export fn globalCleanup();`
- `export fn httpGet(url: string): !CurlResponse;`
