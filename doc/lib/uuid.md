# CP 标准库：分布式唯一标识符引擎手册 (`cap:uuid` / `cap.uuid`)

## 一、 架构总览与设计准则

`cap:uuid` 是 CP 语言官方内置的纯原生分布式唯一标识符引擎，支持 **RFC 9562 UUID (NIL / MAX / v4 / v5 SHA-1 / v7 时间单调有序)**、**128 位 Crockford Base32 ULID** 与 **无偏拒绝采样 NanoID**。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`、零 `zig{}` 块**，基于 Linux `std.os.linux.getrandom` CSPRNG、`std.os.linux.clock_gettime` 墙钟与 `std.crypto.hash.Sha1` 实现，支持 `import ... from "cap:uuid"` 与 `import cap from "cap"; cap.uuid.*` 编译期自动惰性挂载。

`lib/uuid/uuid.cp` 已对照 Rust 工业级标杆仓 [`uuid-rs/uuid`](https://github.com/uuid-rs/uuid)（MIT / Apache-2.0 双许可）完成 RFC 9562 边界加固，并在热路径落地 **16-lane `@Vector(16, u8)` SIMD 加速**（十六进制合法性预检、大小写归一化、16 字节相等性比对与 32 字节十六进制格式化编码）。

### 模块文件结构

| 文件路径 | 谱系与标杆注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/uuid/uuid.cp` | `// Reference-hardened against uuid-rs/uuid (MIT/Apache-2.0)` | 16 字节值类型 `Uuid`（v4/v5/v7/v7At、`NIL`/`MAX`、全 4 种标准文本格式严格解析与格式化、16-lane SIMD 热路径原语、RFC 9562 版本/变体校验与命名空间常量） |
| `lib/uuid/ulid.cp` | `// Source: cp/lib/uuid/ulid.cp (ULID 128-bit Crockford Base32 Sortable ID)` | 16 字节值类型 `Ulid`（Crockford Base32 26 字符编解码、大小写/模糊字符归一、单调生成与 UUID 双向无损转换） |
| `lib/uuid/nanoid.cp` | `// Source: cp/lib/uuid/nanoid.cp (NanoID Unbiased CSPRNG Generator)` | `NanoId` 无偏拒绝采样随机 ID 生成器（默认 21 位 URL 安全字符集，支持自定义字母表与长度） |
| `lib/uuid/index.cp` | `// Reference-hardened against uuid-rs/uuid (MIT/Apache-2.0)` | `cap:uuid` 与 `cap.uuid` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. `Uuid` 与 RFC 9562 常量（`uuid.cp`）

- **内置常量**：
  - `NIL` (`00000000-0000-0000-0000-000000000000`)、`MAX` (`ffffffff-ffff-ffff-ffff-ffffffffffff`)
  - `NAMESPACE_DNS` (`6ba7b810-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_URL` (`6ba7b811-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_OID` (`6ba7b812-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_X500` (`6ba7b814-9dad-11d1-80b4-00c04fd430c8`)
  - 变体常量：`VARIANT_NCS` (0)、`VARIANT_RFC4122` / `VARIANT_RFC9562` (2)、`VARIANT_MICROSOFT` (6)、`VARIANT_FUTURE` (7)
  - 版本常量：`VERSION_NIL` (0)、`VERSION_MAC` (1)、`VERSION_DCE` (2)、`VERSION_MD5` (3)、`VERSION_RANDOM` (4)、`VERSION_SHA1` (5)、`VERSION_SORT_MAC` (6)、`VERSION_SORT_RAND` (7)、`VERSION_CUSTOM` (8)、`VERSION_MAX` (15)
- **构造与生成**：
  - `Uuid.nil()` / `Uuid.max()` / `Uuid.fromBytes(raw)` / `Uuid.fromU128(v)`
  - `Uuid.v4()`：CSPRNG 随机 122 位 + version 4 + RFC 9562 variant (`10xx`)
  - `Uuid.v5(namespace, name)`：RFC 9562 SHA-1 确定性命名空间散列 UUID（已通过 RFC 9562 Appendix A.4 与 `uuid-rs` 全套 16 组向量校验）
  - `Uuid.v7At(unix_ms)`：指定 48 位毫秒时间戳的无状态 UUID v7
  - `Uuid.v7()`：带原子自旋锁保护的严格单调递增 UUID v7（同毫秒内通过 12 位 `rand_a` 递增保障字典序与时间严格单调）
- **严格 4 格式解析与格式化（对照 `uuid-rs/uuid`）**：
  - `Uuid.parse(str): Uuid?`：严格分派 4 种标准文本格式，拒绝 `{32-hex}`、`urn:uuid:<32-hex>`、错位连字符、非法十六进制字符与截断输入：
    - `Uuid.parseSimple(str): Uuid?`（32B 无连字符 hex）
    - `Uuid.parseHyphenated(str): Uuid?`（36B `8-4-4-4-12`）
    - `Uuid.parseBraced(str): Uuid?`（38B `{8-4-4-4-12}`）
    - `Uuid.parseUrn(str): Uuid?`（45B `urn:uuid:8-4-4-4-12`，前缀大小写不敏感）
  - 格式化：`format(buf)`（小写 36B）/ `formatUpper(buf)`（大写 36B）/ `formatCompact(buf)` & `formatSimple(buf)`（小写 32B）/ `formatSimpleUpper(buf)`（大写 32B）/ `formatBraced(buf)`（小写 38B）/ `formatBracedUpper(buf)`（大写 38B）/ `formatUrn(buf)`（小写 45B）/ `formatUrnUpper(buf)`（大写 45B）/ `toStringAlloc(alloc)`
- **元信息与比对**：
  - `isNil()` / `isMax()` / `eql(other)`（均基于 16-lane `@Vector(16, u8)` `@reduce(.And, ...)` SIMD 加速）
  - `version()` / `getVersion(): u8?`（RFC 9562 变体验证版：`NIL`→0、`MAX`→15、RFC9562 变体→1..8，否则 `null`）
  - `variant()` / `getVariant(): u8`
  - `extractV7TimestampMs(): u64?`（严格校验 `version == 7 && variant == VARIANT_RFC9562`）
  - `asBytes(): [16]u8` / `asU128(): u128` / `cmp(other): i32`
- **16-lane SIMD 原语导出**：
  - `simdEql16(a, b)`：16 字节向量相等性比对
  - `simdIsHex16(chunk)` / `simdIsHex32(input)`：16/32 字节 ASCII 十六进制合法性预检
  - `simdAsciiToLower16(chunk)` / `simdNormalizeHexLower32(input, out)`：16/32 字节大小写归一化
  - `simdDecodeHex32(hex)` / `simdFormatHex32(bytes, out, upper)`：32 字节十六进制编解码热路径

### 2. `Ulid`（`ulid.cp`）

- **128 位时间有序结构**：高 48 位为大端毫秒时间戳，低 80 位为 CSPRNG 随机位（`Ulid.generate()` 在同一毫秒内自动大端加 1 保证严格单调）。
- **Crockford Base32 26 字符编解码**：
  - `Ulid.parse(str): Ulid?` / `Ulid.decode(str): Ulid?`：忽略大小写，按 Crockford 规范将 `I`/`L` 映射为 `1`、`O` 映射为 `0`，拒绝非法字符 `U` 与首字符 `> '7'` 的 128 位溢出串。
  - `format(buf)` / `encode(buf)` / `formatLower(buf)` / `toStringAlloc(alloc)`
- **时间与 UUID 互转**：
  - `generateAt(unix_ms)` / `generate()` / `timestampMs()`
  - `toUuid()` / `Ulid.fromUuid(u)`：16 字节 128 位零损双向转换。

### 3. `NanoId`（`nanoid.cp`）

- **常量**：`NANOID_DEFAULT_SIZE = 21`，`NANOID_URL_ALPHABET = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"`（64 字符）。
- **无偏拒绝采样算法**：按 `mask = 2^k - 1 >= alphabet.len - 1` 掩码过滤掉超出 `[0, alphabet.len)` 范围的随机字节，彻底消除任意长度字母表（`1..=256`）的模偏差。
- **生成接口**：
  - `NanoId.generate(buf: *[21]u8)`
  - `NanoId.generateInto(buf: []u8, alphabet: []const u8 = NANOID_URL_ALPHABET): []const u8`
  - `NanoId.generateAlloc(alloc, len = 21): ([]u8)!`
  - `NanoId.generateCustomAlloc(alloc, len, alphabet): ([]u8)!`

---

## 三、 使用示例

```typescript
import cap from "cap";
import { Uuid, Ulid, NanoId, NAMESPACE_DNS, NIL, MAX } from "cap:uuid";

pub fn demoUuid(): !void {
    const id4 = Uuid.v4();
    const id5 = Uuid.v5(NAMESPACE_DNS, "www.example.com");
    const id7 = Uuid.v7();

    let buf36: [36]u8 = undefined;
    let buf38: [38]u8 = undefined;
    let buf45: [45]u8 = undefined;
    const s7 = id7.format(&buf36);
    const braced = id7.formatBraced(&buf38);
    const urn = id7.formatUrn(&buf45);
    const ts_ms = id7.extractV7TimestampMs().?;

    // 严格 4 格式解析
    const parsed_braced = Uuid.parseBraced(braced).?;
    const parsed_urn = Uuid.parseUrn(urn).?;

    const ulid = Ulid.generate();
    const back = Ulid.fromUuid(ulid.toUuid());

    let nano_buf: [21]u8 = undefined;
    const nid = NanoId.generate(&nano_buf);

    // 动态懒加载挂载等价用法
    const hub_id = cap.uuid.Uuid.v7();
}
```
