# CP 标准库：多进制与校验编码套件手册 (`cap:codec`)

## 一、 架构总览与设计准则

`cap:codec` 是 CP 语言官方内置的纯 CP 多进制与校验编码标准库，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:codec"` 静态导入，也支持 `import cap from "cap"; cap.codec.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                   CP 多进制与校验编码套件 (cap:codec & cap.codec)            │
├──────────────────────────────────────┬──────────────────────────────────────┤
│ Base58 / Base85 (base58_85.cp)       │ Bech32 / Crockford32 (bech32_*.cp)   │
│ · Base58 (Bitcoin/IPFS/Solana)       │ · Bech32 / Bech32m (BIP-173/BIP-350) │
│ · Base58Check (复用 cap:crypto SHA256)│ · convertBits / hrpExpand / polymod  │
│ · Adobe ASCII85 (<~...~> + 'z' 压缩) │ · encodeBytes / decodeBytes 8-bit 包装│
│ · RFC 1924 Base85 (4 字节流式分块)   │ · Crockford32 (I/L->1, O->0, 模37校验)│
└──────────────────────────────────────┴──────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/codec/base58_85.cp` | `// Source: cp/lib/codec/base58_85.cp (...)` | `CodecError`、`Base58`（含 `Base58Check`）、`Base85`（Adobe ASCII85 & RFC 1924） |
| `lib/codec/bech32_crockford.cp` | `// Source: cp/lib/codec/bech32_crockford.cp (...)` | `Bech32Variant`、`Bech32Decoded`、`Bech32`、`Crockford32` |
| `lib/codec/index.cp` | `// Source: cp/lib/codec/index.cp (...)` | `cap:codec` 与 `cap.codec` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 统一错误集 `CodecError`

`CodecError` 包含：
- `InvalidCharacter`：输入中包含超出当前字母表或规范范围的字符（如 Base58 中出现 `'0'`/`'O'`/`'I'`/`'l'`、ASCII85 组内出现 `'z'`、Crockford 数据段出现 `'U'`）。
- `InvalidLength`：输入长度不合法（如 Base58Check 解码不足 4 字节、Base85 末尾出现单字符余数 `rem == 1`、Bech32 长度 `< 8` 或 `> 90`、非零尾比特填充）。
- `InvalidChecksum`：校验和校验失败（Base58Check Double-SHA256、Bech32/Bech32m BCH `polymod`、Crockford 模 37 校验符不匹配）。
- `InvalidPrefix`：定界符或前缀格式错误（如 ASCII85 `<~` 与 `~>` 不配对、Bech32 缺少分隔符 `'1'` 或 HRP 为空）。
- `MixedCase`：Bech32 / Bech32m 中同时混用大写与小写 ASCII 字母（违反 BIP-173）。
- `Overflow`：数值溢出（如 Base85 5 字符组解码值 `> 0xFFFFFFFF`、Bech32 5-bit 单词 `>= 32`、Crockford `u64` 超界）。
- `OutOfMemory`：内存分配失败。

### 2. `Base58`（Bitcoin / IPFS / Solana 字母表 & Base58Check）

字母表：`123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz`（剔除 `0`、`O`、`I`、`l`）。

| 方法 | 说明 |
| :--- | :--- |
| `Base58.encode(bytes)` / `encodeAlloc(alloc, bytes)` | 将字节切片编码为 Base58，精确保留所有前导 `0x00` 为 `'1'` |
| `Base58.decode(encoded)` / `decodeAlloc(alloc, encoded)` | 将 Base58 文本解码为原始字节，还原前导 `'1'` 为 `0x00` |
| `Base58.encodeCheck(payload)` / `encodeCheckAlloc(alloc, payload)` | 复用 `cap:crypto` `sha256Raw` 计算 4 字节 Double-SHA256 校验和后 Base58 编码 |
| `Base58.decodeCheck(encoded)` / `decodeCheckAlloc(alloc, encoded)` | 解码并校验 4 字节 Double-SHA256 校验和，篡改任意比特返回 `InvalidChecksum` |

### 3. `Base85`（Adobe ASCII85 & RFC 1924）

| 方法 | 说明 |
| :--- | :--- |
| `Base85.encodeAscii85(bytes)` / `encodeAscii85Alloc(alloc, bytes)` | Adobe ASCII85 编码，输出带 `<~...~>` 边界，4 字节全零块压缩为 `'z'`，支持末尾 1..3 字节精确编码 |
| `Base85.encodeAscii85Raw(bytes)` | 不带 `<~...~>` 定界符的裸 Adobe ASCII85 编码 |
| `Base85.decodeAscii85(encoded)` / `decodeAscii85Alloc(alloc, encoded)` | Adobe ASCII85 解码，支持 `<~...~>` 或裸流、忽略 ASCII 空白、展开 `'z'`、拦截 `> 0xFFFFFFFF` 返回 `Overflow` |
| `Base85.encodeRfc1924(bytes)` / `encodeRfc1924Alloc(alloc, bytes)` | 使用 RFC 1924 字母表进行任意长度 4 字节分块编码 |
| `Base85.decodeRfc1924(encoded)` / `decodeRfc1924Alloc(alloc, encoded)` | 解码 RFC 1924 Base85 文本，校验单字符余数与 `u32` 溢出 |

### 4. `Bech32`（BIP-173 & BIP-350）

- **`Bech32Variant`**：`Bech32 = 1`（BIP-173）、`Bech32m = 0x2bc830a3`（BIP-350）。
- **`Bech32Decoded`**：`hrp: string`、`data: []u8`、`variant: Bech32Variant`。

| 方法 | 说明 |
| :--- | :--- |
| `Bech32.hrpExpand(hrp)` / `hrpExpandAlloc(alloc, hrp)` | 按 BIP-173 展开 HRP 高 3 位 + `0` + 低 5 位 |
| `Bech32.polymod(values)` | 计算 BIP-173 30-bit BCH `polymod` 校验多项式 |
| `Bech32.convertBits(data, from_bits, to_bits, pad)` / `convertBitsAlloc(...)` | 通用 `1..8` 位宽流转换（用于 8-bit 字节与 5-bit 字互转，非零填充严格拦截） |
| `Bech32.encode(hrp, data_5bit, variant)` / `encodeAlloc(...)` | 编码 HRP 与 5-bit 数据段（校验长度 `<= 90`、HRP ASCII `33..126`、无混写） |
| `Bech32.decode(encoded)` / `decodeAlloc(...)` | 解码并自动识别 `Bech32` / `Bech32m` 变体，拒绝大小写混用 `MixedCase` 与错误校验和 |
| `Bech32.encodeBytes(hrp, bytes, variant)` / `decodeBytes(encoded)` | 便捷 8-bit 字节载荷直接编解码包装（内部自动完成 `8 <-> 5` 位转换） |

### 5. `Crockford32`（人类友好防混淆编码 & 模 37 校验符）

字母表：`0123456789ABCDEFGHJKMNPQRSTVWXYZ`（剔除 `I`、`L`、`O`、`U`）。

| 方法 | 说明 |
| :--- | :--- |
| `Crockford32.encode(bytes)` / `encodeAlloc(alloc, bytes)` | 无填充大写 Crockford Base32 编码 |
| `Crockford32.decode(text)` / `decodeAlloc(alloc, text)` | 自动忽略 `'-'` 分隔符、大小写不敏感、容错映射 `I`/`L`->`1` 与 `O`->`0` |
| `Crockford32.encodeU64Check(value)` / `encodeU64CheckAlloc(...)` | 将 `u64` 编码并追加模 37 校验符（`0..31` 及 `*~$=U`） |
| `Crockford32.decodeU64Check(text)` | 忽略 `'-'` 并容错映射 `I`/`L`/`O` 后校验模 37 校验符，不匹配返回 `InvalidChecksum` |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Base58, Base85, Bech32, Bech32Variant, Crockford32 } from "cap:codec";
import cap from "cap";

pub fn demoCodec(): ! {
    // 1. Base58 & Base58Check
    const b58 = Base58.encode("Hello World!")?;            // "2NEpo7TZRRrLZSi2U"
    const chk = Base58.encodeCheck("payload")?;
    const raw = Base58.decodeCheck(chk)?;

    // 2. Adobe ASCII85 & RFC 1924
    const zeros: [4]u8 = [0, 0, 0, 0];
    const a85 = Base85.encodeAscii85(&zeros)?;            // "<~z~>"
    const rfc = Base85.encodeRfc1924("CP-Lang")?;

    // 3. Bech32m 8-bit 字节载荷便捷编解码
    const b32m = Bech32.encodeBytes("cp", "agent-01", Bech32Variant.Bech32m)?;
    const dec = Bech32.decodeBytes(b32m)?;

    // 4. Crockford Base32 容错解码与模 37 校验符 (通过 cap.codec 动态挂载)
    const num_chk = cap.codec.Crockford32.encodeU64Check(1234)?; // "16JD"
    const restored = cap.codec.Crockford32.decodeU64Check("16-jD")?;
    * = b58;
    * = raw;
    * = a85;
    * = rfc;
    * = dec;
    * = restored;
}
```
