# CP 标准库：NanoID、单调 ULID 与 RFC 9562 UUID v7 手册 (`cap:nanoid`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 一、 架构总览与设计准则

`cap:nanoid` 是 CP 语言官方内置的纯 CP **高性能唯一标识符生成与解析库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。所有随机数均通过显式注入的确定性 SplitMix64 伪随机数生成器 `IdRng` 提供，所有毫秒时间戳均通过 `timestamp_ms: u64` 参数显式传入，确保分布式系统与单元测试 100% 确定性可复现。支持 `import { ... } from "cap:codec"` 与 `import cap from "cap"; cap.nanoid.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/nanoid/nanoid.cp` | `// Source: cp/lib/nanoid/nanoid.cp (...)` | `IdError`、`IdRng`、`NanoId`、`Ulid`、`MonotonicUlidGenerator`、`UuidV7` 核心实现 |
| `lib/nanoid/index.cp` | `// Source: cp/lib/nanoid/index.cp (...)` | `cap:nanoid` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `IdError`

- `InvalidLength`：NanoID 请求长度为 `0`。
- `InvalidAlphabet`：自定义字母表为空或长度超过 `256` 字节。
- `InvalidUlid`：ULID 字符串长度非 26、包含非法字符（如 `'U'`）或首字符大于 `'7'` 导致 128-bit 溢出。
- `InvalidUuid`：UUID v7 字符串长度非 36、连字符位置错误、包含非十六进制字符，或 version != 7 / variant != 2。
- `MonotonicOverflow`：`MonotonicUlidGenerator` 在同一毫秒内 80-bit 随机数达到全 `0xFF` 上限无法继续递增。
- `TimestampOutOfRange`：`timestamp_ms` 超出 48-bit 无符号整数范围（`> 0xFFFF_FFFF_FFFF`）。
- `OutOfMemory`：内存分配失败。

### 2. `IdRng` 确定性 PRNG

- **`IdRng.init(seed: u64): IdRng`**：使用 64-bit 种子初始化 SplitMix64 状态。
- **`nextU64(*_): u64`**：步进状态并返回下一个 64-bit 伪随机整数。
- **`fillBytes(*_, buf: []u8)`**：将伪随机字节填充至目标切片 `buf`。

### 3. `NanoId` 无偏差生成器

- **`NanoId.DEFAULT_ALPHABET`**：64 字符 URL 安全字母表 `_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`。
- **`NanoId.DEFAULT_SIZE`**：默认长度 `21`。
- **`NanoId.generate(allocator: std.mem.Allocator, rng: *IdRng, len: usize): ([]u8) ! IdError`**：使用默认 64 字符字母表生成长度为 `len` 的 NanoID（调用方负责释放返回的切片）。
- **`NanoId.generateCustom(allocator: std.mem.Allocator, rng: *IdRng, alphabet: []const u8, len: usize): ([]u8) ! IdError`**：使用位掩码拒绝采样（Rejection Sampling）消除模数偏差，支持 `1..=256` 字节长度的自定义字母表。

### 4. `Ulid` 与 `MonotonicUlidGenerator`

- **`Ulid.fromParts(timestamp_ms: u64, rand80: u128): Ulid ! IdError`**：由 48-bit 毫秒时间戳与 80-bit 随机数构造 ULID。
- **`Ulid.generate(rng: *IdRng, timestamp_ms: u64): Ulid ! IdError`**：生成指定毫秒时间戳的随机 ULID。
- **`encode(): [26]u8`**：编码为 26 字符 Crockford Base32 数组（字典序与时间序严格一致）。
- **`Ulid.decode(str: []const u8): Ulid ! IdError`**：解码 26 字符 Crockford Base32 字符串，支持大小写不敏感及 `I/L -> 1`、`O -> 0` 容错，拦截首字符 `> '7'` 的 128-bit 溢出。
- **`timestampMs(): u64`** / **`rand80(): u128`**。
- **`MonotonicUlidGenerator.init(): MonotonicUlidGenerator`** / **`fromState(last_timestamp_ms: u64, last_rand80: u128): MonotonicUlidGenerator ! IdError`**：
  - **`next(*_, rng: *IdRng, timestamp_ms: u64): Ulid ! IdError`**：若时间戳推进则重新采样 80-bit 随机数；若仍在同一毫秒（或时钟轻微回拨）则将 80-bit 随机数加 1，保证严格字典序单调递增，全 `0xFF` 时返回 `IdError.MonotonicOverflow`。

### 5. `UuidV7` (RFC 9562)

- **`UuidV7.generate(rng: *IdRng, timestamp_ms: u64): UuidV7 ! IdError`**：按 RFC 9562 布局生成 48-bit Unix ms + version `0b0111` + 12-bit `rand_a` + variant `0b10` + 62-bit `rand_b`。
- **`format(): [36]u8`**：格式化为标准小写 `8-4-4-4-12` 36 字节字符串。
- **`UuidV7.parse(str: []const u8): UuidV7 ! IdError`**：严格校验 36 字符格式、连字符位置、`version == 7` 与 `variant == 2`。
- **`timestampMs(): u64`** / **`version(): u8`** / **`variant(): u8`**。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { IdRng, NanoId, Ulid, MonotonicUlidGenerator, UuidV7 } from "cap:codec";

pub fn demoIds(): ! {
    const alloc = std.heap.page_allocator;
    let rng = IdRng.init(42);

    const nid = NanoId.generate(alloc, &rng, NanoId.DEFAULT_SIZE)?;
    defer alloc.free(nid);

    let mono = MonotonicUlidGenerator.init();
    const u1 = mono.next(&rng, 1700000000000)?;
    const u2 = mono.next(&rng, 1700000000000)?;
    const enc1 = u1.encode();
    const enc2 = u2.encode();
    * = enc1;
    * = enc2;

    const v7 = UuidV7.generate(&rng, 1700000000000)?;
    const s = v7.format();
    const parsed = UuidV7.parse(&s)?;
    * = parsed.timestampMs();
}
```
