# CP 标准库：分布式唯一标识符引擎手册 (`cap:uuid` / `cap.uuid`)

## 一、 架构总览与设计准则

`cap:uuid` 是 CP 语言官方内置的纯原生分布式唯一标识符引擎，支持 **RFC 9562 UUID (v4 / v5 SHA-1 / v7 时间单调有序)**、**128 位 Crockford Base32 ULID** 与 **无偏拒绝采样 NanoID**。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，基于 Linux `std.os.linux.getrandom` CSPRNG、`std.os.linux.clock_gettime` 墙钟与 `std.crypto.hash.Sha1` 实现，支持 `import ... from "cap:uuid"` 与 `import cap from "cap"; cap.uuid.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/uuid/uuid.cp` | `// Source: cp/lib/uuid/uuid.cp (RFC 9562 UUID v4, v5 SHA-1 & v7 Time-Ordered)` | 16 字节值类型 `Uuid`（v4/v5/v7/v7At、`nil`/`max`、多格式解析与格式化、RFC 9562 命名空间常量） |
| `lib/uuid/ulid.cp` | `// Source: cp/lib/uuid/ulid.cp (ULID 128-bit Crockford Base32 Sortable ID)` | 16 字节值类型 `Ulid`（Crockford Base32 26 字符编解码、大小写/模糊字符归一、单调生成与 UUID 双向无损转换） |
| `lib/uuid/nanoid.cp` | `// Source: cp/lib/uuid/nanoid.cp (NanoID Unbiased CSPRNG Generator)` | `NanoId` 无偏拒绝采样随机 ID 生成器（默认 21 位 URL 安全字符集，支持自定义字母表与长度） |
| `lib/uuid/index.cp` | `// Source: cp/lib/uuid/index.cp (Unified Exports for cap:uuid)` | `cap:uuid` 与 `cap.uuid` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. `Uuid` 与 RFC 9562 常量（`uuid.cp`）

- **内置命名空间常量**：
  - `NAMESPACE_DNS` (`6ba7b810-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_URL` (`6ba7b811-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_OID` (`6ba7b812-9dad-11d1-80b4-00c04fd430c8`)
  - `NAMESPACE_X500` (`6ba7b814-9dad-11d1-80b4-00c04fd430c8`)
- **构造与生成**：
  - `Uuid.nil()` / `Uuid.max()` / `Uuid.fromBytes(raw)`
  - `Uuid.v4()`：CSPRNG 随机 122 位 + version 4 + RFC 9562 variant (`10xx`)
  - `Uuid.v5(namespace, name)`：RFC 9562 SHA-1 确定性命名空间散列 UUID
  - `Uuid.v7At(unix_ms)`：指定 48 位毫秒时间戳的无状态 UUID v7
  - `Uuid.v7()`：带原子自旋锁保护的严格单调递增 UUID v7（同毫秒内通过 12 位 `rand_a` 递增保障字典序与时间严格单调）
- **解析、校验与格式化**：
  - `Uuid.parse(str): Uuid?`：支持 36 字符标准连字符、32 字符紧凑 hex、`urn:uuid:`（大小写不敏感）前缀与 `{...}` 花括号格式
  - `isNil()` / `isMax()` / `version()` / `variant()` / `extractV7TimestampMs(): u64?` / `eql(other)` / `cmp(other)`
  - `format(buf)`（小写 36 字符）/ `formatUpper(buf)`（大写 36 字符）/ `formatCompact(buf)`（32 字符）/ `formatUrn(buf)`（45 字符）/ `toStringAlloc(alloc)`

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
import { Uuid, Ulid, NanoId, NAMESPACE_DNS } from "cap:uuid";

pub fn demoUuid(): !void {
    const id4 = Uuid.v4();
    const id5 = Uuid.v5(NAMESPACE_DNS, "www.example.com");
    const id7 = Uuid.v7();

    let buf36: [36]u8 = undefined;
    const s7 = id7.format(&buf36);
    const ts_ms = id7.extractV7TimestampMs().?;

    const ulid = Ulid.generate();
    const back = Ulid.fromUuid(ulid.toUuid());

    let nano_buf: [21]u8 = undefined;
    const nid = NanoId.generate(&nano_buf);

    // 动态懒加载挂载等价用法
    const hub_id = cap.uuid.Uuid.v7();
}
```
