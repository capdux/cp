# `cap:ulid` / `cap.ulid` — 工业级 ULID 编解码与单调生成器

纯 CP 实现的零堆分配 ULID（Universally Unique Lexicographically Sortable Identifier）标准库，完整支持 128-bit 大端布局（48-bit 毫秒时间戳 + 80-bit 随机熵）、Crockford Base32 编解码、UUID 双向无损互转及同毫秒严格单调递增生成器。

## 快速导入

```cp
import { Ulid, MonotonicUlidGenerator, UlidError } from "cap:ulid";
// 或通过统一入口懒挂载
import cap from "cap";
// cap.ulid.Ulid / cap.ulid.MonotonicUlidGenerator
```

## 错误类型 (`UlidError`)

- `InvalidLength`: 字符串长度不符合 26 字符（或忽略连字符后有效字符数不为 26）。
- `InvalidChar`: 包含非 Crockford Base32 字符（如 `U`、`u` 等），或严格模式下包含连字符/`I`/`L`/`O`。
- `TimestampOverflow`: 时间戳超过 48-bit 上限 (`0x0000_FFFF_FFFF_FFFF`)，或解码首字符 `> '7'` 导致 128-bit 溢出。
- `RandomOverflow`: 随机熵边界溢出。
- `InvalidUuid`: 非法的 `8-4-4-4-12` 36 字符 UUID 字符串。
- `MonotonicOverflow`: 同毫秒内 80-bit 随机部分全 `0xFF` 无法继续递增。

## 核心 API

### `Ulid`

- `Ulid.nil(): Ulid` / `Ulid.max(): Ulid`
- `Ulid.fromBytes(bytes: [16]u8): Ulid`
- `Ulid.fromParts(timestamp_ms: u64, ent: [10]u8): Ulid ! UlidError`
- `u.toBytes(): [16]u8`
- `u.timestampMs(): u64` / `u.entropy(): [10]u8`
- `u.compare(other: Ulid): i8` (`-1`, `0`, `1`) / `u.equals(other: Ulid): bool` / `u.isZero(): bool`
- `u.encode(): [26]u8` / `u.encodeLower(): [26]u8`（零堆分配）
- `Ulid.decode(str: []const u8): Ulid ! UlidError`（容错：大小写、`I/L->1`、`O->0`、可选 `-` 连字符，校验首字符 `<= '7'`）
- `Ulid.decodeStrict(str: []const u8): Ulid ! UlidError`（严格：长度 26、无连字符、无 `I/L/O` 歧义字符）
- `u.toUuidString(): [36]u8`（标准 `8-4-4-4-12` 小写十六进制）
- `Ulid.fromUuidString(s: []const u8): Ulid ! UlidError`

### `MonotonicUlidGenerator`

- `MonotonicUlidGenerator.init(seed_bytes_opt: ([10]u8)?): MonotonicUlidGenerator`
- `gen.generateAt(now_ms: u64): Ulid ! UlidError`（同毫秒内 80-bit 熵 `+1`，溢出返回 `MonotonicOverflow`；新毫秒自动重生成 CSPRNG 熵）
- `gen.generate(): Ulid ! UlidError`（使用当前系统毫秒时间戳）

- `Ulid.fromU128(val: u128): Ulid` / `u.toU128(): u128`（128 位大端无符号整数无损互转，对标 Rust `ulid` crate）
- `u.increment(): Ulid?`（同毫秒内 80-bit 熵 `+1`，溢出返回 `null`，对标 Rust `Ulid::increment()`）

## 使用示例

### 示例 1：单调递增 ULID 生成与 Crockford Base32 编码

```cp
import { Ulid, MonotonicUlidGenerator } from "cap:ulid";

fn demoGenerate() {
    let gen = MonotonicUlidGenerator.init(null);
    const id1 = gen.generateAt(1700000000000) catch return;
    const id2 = gen.generateAt(1700000000000) catch return;

    // 同一毫秒内严格递增：id1 < id2
    const cmp = id1.compare(id2); // -1
    const text = id1.encode();    // [26]u8，例如 "01HF7..."
    * = cmp;
    * = text;
}
```

### 示例 2：容错解码、严格解码与 `u128` / `UUID` 无损互转

```cp
import { Ulid } from "cap:ulid";

fn demoConvert() {
    // 容错解码：支持小写、连字符及 I/L -> 1、O -> 0
    const u = Ulid.decode("01ar3z-ndekts-v4rrff-q69g5f-av") catch return;
    const raw_u128 = u.toU128();
    const u2 = Ulid.fromU128(raw_u128); // 与 u 字节完全一致

    // 无损互转为标准 36 字符 UUID
    const uuid = u2.toUuidString();
    const back = Ulid.fromUuidString(uuid[0..]) catch return;
    const same = back.equals(u); // true
    * = same;
}
```

### 示例 3：同毫秒确定性递增（`increment()`）与边界溢出防护

```cp
import { Ulid } from "cap:ulid";

fn demoIncrement() {
    const base = Ulid.fromParts(1700000000000, [0; 10]) catch return;
    const next = base.increment().?; // 熵末字节变为 1，时间戳不变

    // 最大值 80-bit 全 0xFF 递增返回 null（防溢出污染时间戳）
    const overflow = Ulid.max().increment(); // null
    * = next;
    * = overflow;
}
```
