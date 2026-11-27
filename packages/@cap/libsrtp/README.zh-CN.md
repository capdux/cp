[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/libsrtp

> 版本：`2.6.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Cisco libsrtp2 2.6.0 SRTP/SRTCP encrypted media transport native C binding for CP

## 安装与导入

```bash
cap add @cap/libsrtp
```

```cp
import { cp_srtp_init } from "@cap/libsrtp";
```

## 核心导出 API

- `export fn cp_srtp_init(): i32;`
- `export fn cp_srtp_shutdown(): i32;`
- `export fn cp_srtp_version(): [*:0]&u8!?;`
- `export fn cp_srtp_create_aes128_sha1(key30: [*]&u8!, inbound: i32): *auto!?;`
- `export fn cp_srtp_protect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;`
- `export fn cp_srtp_unprotect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;`
- `export fn cp_srtp_destroy(session: *auto!?);`
- `export fn mapSrtpStatus(code: i32, is_unprotect: bool): SrtpError;`
- `export fn srtpInit(): !void;`
- `export fn srtpShutdown(): !void;`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`libsrtp@2.6.0`)，许可证：BSD-3-Clause
