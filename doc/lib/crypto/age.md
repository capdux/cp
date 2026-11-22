# `cap:age` — 零分配 age v1 现代文件加密格式解析标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

`cap:age` 是纯 CP、零堆分配的 `age-encryption.org/v1` 文件格式解析与校验库，支持严格 LF 版本行嗅探、ASCII Armor 检测、多 Recipient Stanza 零分配迭代（含 64 列折行无填充 Base64 body 规范校验）、HMAC-SHA256 输入区 `header_for_mac` / 43 字节 MAC / STREAM 16 字节 Nonce 提取，以及 Bech32 风格 `age1...` 公钥与 `AGE-SECRET-KEY-1...` 私钥格式校验。

---

## 快速导入

```typescript
import {
    AGE_VERSION_LINE,
    AGE_ARMOR_BEGIN,
    AGE_ARMOR_END,
    AGE_STREAM_CHUNK_SIZE,
    AGE_STREAM_TAG_SIZE,
    AGE_NONCE_SIZE,
    AgeStanza,
    AgeHeader,
    AgeStanzaIter,
    isAge,
    isAgeArmored,
    parseAgeHeader,
    isValidAgePublicKey,
    isValidAgeSecretKey,
} from "cap:crypto";
```

---

## 常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `AGE_VERSION_LINE` | `"age-encryption.org/v1"` | age v1 标准版本行 |
| `AGE_ARMOR_BEGIN` | `"-----BEGIN AGE ENCRYPTED FILE-----"` | ASCII Armor 起始边界 |
| `AGE_ARMOR_END` | `"-----END AGE ENCRYPTED FILE-----"` | ASCII Armor 结束边界 |
| `AGE_STREAM_CHUNK_SIZE` | `65536` | STREAM 分块大小（64 KiB） |
| `AGE_STREAM_TAG_SIZE` | `16` | STREAM Poly1305 标签长度 |
| `AGE_NONCE_SIZE` | `16` | STREAM 前导 Nonce 长度 |

---

## 核心数据结构与函数

### `AgeStanza` & `AgeStanzaIter`
- `AgeStanza`：`stanza_type: []const u8`、`args_slice: []const u8`、`body_b64: []const u8`；只读方法 `argCount(): usize`、`argAt(idx: usize): ([]const u8)?`。
- `AgeStanzaIter.init(stanzas_slice)` / `next(*_): AgeStanza?`：零分配迭代全部 `-> ...` Stanza。

### `AgeHeader` & `parseAgeHeader(data)`
- `header_for_mac: []const u8`（偏移 `0` 到 `"---"` 含 `"---"`，即官方 HMAC-SHA256 输入）
- `mac_b64: []const u8`（严格 43 字节无填充 Base64 MAC）
- `stanza_count: u32`、`stanzas_slice: []const u8`、`payload_offset: usize`、`payload: []const u8`
- 只读方法：`payloadNonce(): ([]const u8)?`（提取前 16B STREAM nonce）

### 嗅探与密钥格式校验
- `isAge(data)`：首行精确为 `"age-encryption.org/v1\n"`（拒绝 `\r\n`）。
- `isAgeArmored(data)`：跳过前导空白后匹配 `AGE_ARMOR_BEGIN`。
- `isValidAgePublicKey(s)`：`"age1"` + 58 小写 Bech32 字符（62B）。
- `isValidAgeSecretKey(s)`：`"AGE-SECRET-KEY-1"` + 58 大写 Bech32 字符（74B）。
