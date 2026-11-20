# CP 变更日志 (CHANGELOG) - 2026-11-20 (热数据)

> 历史变更记录已完整归档至 [CHANGELOG_H.md](file:///data/dev/cp/CHANGELOG_H.md)。本文件仅保留最新活跃记录。

## [2026-11-21] - 新建 `cap:wireguard` 纯 CP WireGuard Noise_IKpsk2 握手状态机、TAI64N 单调时间戳与 RFC 6479 2048-bit 滑动窗口防重放引擎（支持 `cap.wireguard` 自动动态挂载，详见 `doc/lib/wireguard.md`）（5/5 PASS，自动动态挂载 `cap.wireguard`）
- **新增标准库**：`lib/wireguard/{replay,noise,index}.cp`，实现零堆 2048-bit `ReplayWindow`、`Tai64n` 编解码与完整 `Noise_IKpsk2`（X25519+BLAKE2s+ChaCha20Poly1305）端到端握手与双向传输帧加解密。
- **新增文档与测试**：`doc/lib/wireguard.md` 扁平文档与 `cap/test/test_wireguard_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:arrow_ipc` 纯 CP Apache Arrow IPC Streaming 与 `"ARROW1"` File Format 列式内存批处理及零拷贝读写引擎（支持 `cap.arrow_ipc` 自动动态挂载，详见 `doc/lib/arrow_ipc.md`）（5/5 PASS，自动动态挂载 `cap.arrow_ipc`）
- **新增标准库**：`lib/arrow_ipc/{types,flatbuf,ipc,index}.cp`，实现 5 种列类型（含 LSB-first `ValidityBitmap` 与 32-bit offset `Utf8`）、轻量确定性 Flatbuffer `Schema`/`RecordBatch`/`Footer` 编解码、8 字节对齐 Streaming (`0xFFFFFFFF` EOS) 与 `"ARROW1"` File Format 读写。
- **新增文档与测试**：`doc/lib/arrow_ipc.md` 扁平文档与 `cap/test/test_arrow_ipc_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:ssh_wire` 纯 CP RFC 4251 SSH-2 Wire 编解码、OpenSSH 公私钥解析与 SSHSIG 签名验签引擎（支持 `cap.ssh_wire` 自动动态挂载，详见 `doc/lib/ssh_wire.md`）（5/5 PASS，自动动态挂载 `cap.ssh_wire`）
- **新增标准库**：`lib/ssh_wire/{wire,keys,openssh_key,sshsig,index}.cp`，实现 RFC 4251 `SshReader`/`SshWriter`（含 `mpint` 规范补码）、`ssh-ed25519`/`ecdsa-sha2-nistp256` 公钥与 `SHA256:` 指纹、`openssh-key-v1` 私钥解析及 `SSHSIG` PEM 签名验签。
- **新增文档与测试**：`doc/lib/ssh_wire.md` 扁平文档与 `cap/test/test_ssh_wire_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:parquet` 纯 CP Apache Parquet 列式容器读写、Thrift TCompactProtocol 与 RLE/Bit-Packed Hybrid 编解码引擎（支持 `cap.parquet` 自动动态挂载，详见 `doc/lib/parquet.md`）（5/5 PASS，自动动态挂载 `cap.parquet`）
- **新增标准库**：`lib/parquet/{types,thrift,rle,codec,index}.cp`，实现 ZigZag Varint 与 32 层深度防护 `ThriftReader`/`ThriftWriter`、Parquet RLE/Bit-Packed 混合编解码、`PAR1` 容器校验及 `INT32`/`INT64`/`DOUBLE`/`BYTE_ARRAY`/`BOOLEAN` 列 `writeParquet`/`readParquet`。
- **新增文档与测试**：`doc/lib/parquet.md` 扁平文档与 `cap/test/test_parquet_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

## [2026-11-21] - 新建 `cap:webauthn` 纯 CP FIDO2/WebAuthn Passkey 服务端注册与登录断言验证引擎（复用 `cap:cose`，详见 `doc/lib/webauthn.md`）（5/5 PASS，自动动态挂载 `cap.webauthn`）
- **新增标准库**：`lib/webauthn/{types,auth_data,client_data,verify,index}.cp`（复用 `cap:cose`），实现 `AuthenticatorData` 二进制拆包与 `COSE_Key` 提取、`ClientDataJSON` 常量时间 Challenge/Origin 校验、`verifyRegistrationNone` 与支持 ES256 DER / EdDSA 64B 验签及 `signCount` 防克隆检测的 `verifyAssertion`。
- **新增文档与测试**：`doc/lib/webauthn.md` 扁平文档与 `cap/test/test_webauthn_production_deep.cp`（5/5 PASS，经 `verify_module.sh` 校验通过）。

