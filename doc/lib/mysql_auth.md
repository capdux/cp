# `cap:mysql_auth` — MySQL 握手认证加密与 SSLRequest 编解码

> 溯源：从 `/data/dev/bun/src/sql/mysql/protocol/Auth.cp` 与 `SSLRequest.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:mysql_auth` 提供零外部依赖、零堆分配的 MySQL 认证密码学原语与 `SSLRequest` 帧编解码器：

- **`mysql_native_password`**（SHA-1）：`SHA1(pw) XOR SHA1(salt || SHA1(SHA1(pw)))`（20 字节）；
- **`caching_sha2_password`**（SHA-256）：`SHA256(pw) XOR SHA256(SHA256(SHA256(pw)) || salt)`（32 字节）；
- **`mysql_clear_password`** NUL 结尾明文编码与 RSA-OAEP 前置循环异或 `xorPasswordWithNonce`；
- **`SSLRequest`** 36 字节完整帧（4B 包头 + 32B 负载）双向编解码。

---

## 快速导入

```cp
import {
    AuthMethod,
    AuthFastStatus,
    SSLRequest,
    Error,
    NATIVE_SCRAMBLE_LEN,
    CACHING_SHA2_SCRAMBLE_LEN,
    SSL_REQUEST_FRAME_LEN,
    CAP_CLIENT_SSL,
    scrambleNativePassword,
    scrambleCachingSha2Password,
    encodeCleartextPassword,
    xorPasswordWithNonce,
} from "cap:mysql_auth";
```

---

## API 参考

| 符号 / 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `AuthMethod` | `mysql_native_password`, `caching_sha2_password`, `mysql_clear_password`, `sha256_password` | 提供 `fromPluginName()`、`pluginName()`、`requiresTls()` |
| `AuthFastStatus` | `success = 0x03`, `continue_auth = 0x04` | 提供 `fromByte()` |
| `scrambleNativePassword` | `(password: string, salt: []const u8, out: []u8): ([]const u8)!` | SHA-1 握手加密（空密码返回 `out[0..0]`） |
| `scrambleCachingSha2Password` | `(password: string, salt: []const u8, out: []u8): ([]const u8)!` | SHA-256 握手加密（空密码返回 `out[0..0]`） |
| `encodeCleartextPassword` | `(password: string, out: []u8): ([]const u8)!` | 编码 NUL 结尾明文密码 |
| `xorPasswordWithNonce` | `(password: string, nonce: []const u8, out: []u8): ([]const u8)!` | NUL 结尾密码与盐值循环异或 |
| `SSLRequest` | `{ capability_flags, max_packet_size = 0xFFFFFF, character_set = 45, sequence_id = 1 }` | `encode(out)` / `decode(frame)` / `hasSslCapability()` / `eql(other)` |

---

## 使用示例

```cp
import { scrambleCachingSha2Password, SSLRequest, CAP_CLIENT_SSL } from "cap:mysql_auth";

fn authExample(salt: []const u8)! {
    let out: [32]u8 = [0; 32];
    const token = scrambleCachingSha2Password("s3cr3t", salt, out[0..32])?;
    * = token;

    let frame: [36]u8 = [0; 36];
    const req = SSLRequest{ capability_flags: CAP_CLIENT_SSL | 0x0200 };
    const encoded = req.encode(frame[0..36])?;
    * = encoded;
}
```
