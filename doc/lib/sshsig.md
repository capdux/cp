# `cap:sshsig` — 零分配 OpenSSH `SSHSIG` v1 文件与 Git Commit 签名解析库

> **模块路径**：`lib/sshsig/`（入口 `lib/sshsig/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 OpenSSH `PROTOCOL.sshsig` v1 规范：校验 6 字节 `"SSHSIG"` 魔数与大端 `version == 1`，支持 PEM Armor（`-----BEGIN/END SSH SIGNATURE-----`）严格 Base64 解码、零拷贝大端 SSH wire string 解析（含嵌套 `key_type`、`sig_type` 与 `raw_signature` 提取），以及待签名消息前置数据（`SSHSIG` Signed Data）零分配构造。

---

## 一、导入方式

```cp
import {
    SSHSIG_MAGIC,
    SSHSIG_VERSION,
    SSHSIG_ARMOR_BEGIN,
    SSHSIG_ARMOR_END,
    SshsigBlob,
    isSshsigBinary,
    isSshsigArmored,
    decodeSshsigArmor,
    parseSshsigBinary,
    buildSshsigSignedData,
} from "cap:sshsig";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| `SSHSIG_MAGIC` / `SSHSIG_VERSION` | `"SSHSIG"` / `1` | 二进制魔数与协议版本号 |
| `SSHSIG_ARMOR_BEGIN` / `SSHSIG_ARMOR_END` | `"-----BEGIN SSH SIGNATURE-----"` / `"-----END SSH SIGNATURE-----"` | PEM Armor 边框标记 |
| `SshsigBlob` | `version`, `publickey_blob`, `key_type`, `namespace`, `reserved`, `hash_algorithm`, `signature_blob`, `sig_type`, `raw_signature` | 零拷贝 `SSHSIG` v1 签名结构视图 |

---

## 三、核心 API

- `isSshsigBinary(data: []const u8): bool` — 校验 `data.len >= 10`、前 6B `"SSHSIG"` 且 4B BE `version == 1`。
- `isSshsigArmored(data: []const u8): bool` — 跳过前导空白后校验包含完整 `BEGIN`/`END` 边框。
- `decodeSshsigArmor(armored: []const u8, out: []u8): usize?` — 剥离 PEM 边框并严格解码中间标准 Base64 字节流至 `out`。
- `parseSshsigBinary(data: []const u8): SshsigBlob?` — 按 `PROTOCOL.sshsig` 顺序零拷贝解析并校验非空 `namespace` 与零尾随垃圾字节。
- `buildSshsigSignedData(namespace, reserved, hash_alg, hash_digest, out: []u8): usize?` — 构造 `"SSHSIG" || string(namespace) || string(reserved) || string(hash_alg) || string(hash_digest)` 待验签载荷。

---

## 四、使用示例

```cp
import { decodeSshsigArmor, parseSshsigBinary, buildSshsigSignedData } from "cap:sshsig";

fn inspectSshsig(armored: []const u8, digest: []const u8) {
    let raw_buf: [1024]u8 = [0; 1024];
    if (raw_len := decodeSshsigArmor(armored, raw_buf[0..1024])) {
        if (sig := parseSshsigBinary(raw_buf[0..raw_len])) {
            let preimage: [256]u8 = [0; 256];
            if (n := buildSshsigSignedData(sig.namespace, sig.reserved, sig.hash_algorithm, digest, preimage[0..256])) {
                * = preimage[0..n];
                * = sig.raw_signature;
            }
        }
    }
}
```
