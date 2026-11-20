# CP 研发日志 (DEVLOG) - 2026-11-20 (热数据)

> 历史研发日志已完整归档至 [DEVLOG_H.md](file:///data/dev/cp/DEVLOG_H.md)。本文件仅保留最新活跃记录。

## 2026-11-21: 新建 `cap:raft` 纯 CP Sans-IO Raft 分布式共识状态机、日志复制、冲突回退与快照截断引擎（支持 `cap.raft` 自动动态挂载，详见 `doc/lib/raft.md`）（5/5 PASS，文档直写 `doc/lib/raft.md`）
- **核心设计**：Sans-IO 架构将共识状态机与网络/存储解耦，所有输出以 `RaftAction` 动作列表返回，天然适配内存集群确定性仿真与 CP Actor 信箱驱动，全路径 `std.testing.allocator` 零泄漏。

## 2026-11-21: 新建 `cap:mcp` 纯 CP Model Context Protocol (MCP 2025-03-26) Agent 工具/资源/提示词路由与 NDJSON/Content-Length 增量分帧引擎（支持 `cap.mcp` 自动动态挂载，详见 `doc/lib/mcp.md`）（5/5 PASS，文档直写 `doc/lib/mcp.md`）
- **核心设计**：`McpStreamFramer` 同时支持 NDJSON 与 LSP `Content-Length` 双传输分帧，并在分配帧缓冲前拦截超限载荷；`McpServer` 自动将业务 handler 错误转为协议合规的 `isError: true` 工具结果，全路径零泄漏。

## 2026-11-21: 新建 `cap:wireguard` 纯 CP WireGuard Noise_IKpsk2 握手状态机、TAI64N 单调时间戳与 RFC 6479 2048-bit 滑动窗口防重放引擎（支持 `cap.wireguard` 自动动态挂载，详见 `doc/lib/wireguard.md`）（5/5 PASS，文档直写 `doc/lib/wireguard.md`）
- **核心设计**：采用先验密 AEAD 再更新 `ReplayWindow` 策略杜绝伪造包污染窗口，对 `mac1` 与公钥执行常量时间比对并强制 `TAI64N` 单调递增防重放，全路径零泄漏。

## 2026-11-21: 新建 `cap:arrow_ipc` 纯 CP Apache Arrow IPC Streaming 与 `"ARROW1"` File Format 列式内存批处理及零拷贝读写引擎（支持 `cap.arrow_ipc` 自动动态挂载，详见 `doc/lib/arrow_ipc.md`）（5/5 PASS，文档直写 `doc/lib/arrow_ipc.md`）
- **核心设计**：内置确定性 Flatbuffer 读写器避免外部依赖，所有 `Buffer` 偏移/长度、`Footer` 边界与 `Utf8` offsets 均执行防越界拦截，全路径通过 `std.testing.allocator` 零泄漏验证。

## 2026-11-21: 新建 `cap:ssh_wire` 纯 CP RFC 4251 SSH-2 Wire 编解码、OpenSSH 公私钥解析与 SSHSIG 签名验签引擎（支持 `cap.ssh_wire` 自动动态挂载，详见 `doc/lib/ssh_wire.md`）（5/5 PASS，文档直写 `doc/lib/ssh_wire.md`）
- **核心设计**：严格执行 RFC 4251 `mpint` 高位 `0x80` 前导 `0x00` 补码、`openssh-key-v1` `checkint1 == checkint2` 与 8 字节对齐校验，以及 `SSHSIG` `"SSHSIG"` 魔数与 `namespace` 绑定验签，全路径零泄漏。

## 2026-11-21: 新建 `cap:parquet` 纯 CP Apache Parquet 列式容器读写、Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码引擎（支持 `cap.parquet` 自动动态挂载，详见 `doc/lib/parquet.md`）（5/5 PASS，文档直写 `doc/lib/parquet.md`）
- **核心设计**：内置独立 Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码器实现零外部依赖 Parquet 列式读写，严格拦截缺失 `PAR1` 魔数、越界 Footer 长度与截断页，全路径零泄漏。

## 2026-11-21: 新建 `cap:webauthn` 纯 CP FIDO2/WebAuthn Passkey 服务端注册与登录断言验证引擎（复用 `cap:cose`，详见 `doc/lib/webauthn.md`）（5/5 PASS，文档直写 `doc/lib/webauthn.md`）
- **核心设计**：直接复用 `cap:cose` 解码 `AttestedCredentialData` 公钥；对 `challenge` 采用常量时间比对、`signCount` 实施防克隆递增拦截，全路径通过 `std.testing.allocator` 零泄漏验证。

