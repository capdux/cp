[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/usrsctp

> 版本：`0.9.5` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Portable userland SCTP stack (usrsctp 0.9.5.0) for WebRTC DataChannel in CP

## 安装与导入

```bash
cap add @cap/usrsctp
```

```cp
import { cp_usrsctp_init } from "@cap/usrsctp";
```

## 核心导出 API

- `export fn cp_usrsctp_init(udp_port: u16): i32;`
- `export fn cp_usrsctp_finish(): i32;`
- `export fn cp_usrsctp_socket_create(domain: i32, sock_type: i32): *auto!?;`
- `export fn cp_usrsctp_socket_close(sock: *auto!?);`
- `export fn cp_usrsctp_socket_create_and_close(): i32;`
- `export fn sctpInit(udp_port: u16): !void;`
- `export fn sctpFinish(): !void;`
- `export fn socketCreateAndClose(): !void;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`usrsctp@0.9.5`)，许可证：BSD-3-Clause
