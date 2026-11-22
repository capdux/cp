# `cap:vlq` — SourceMap Base64-VLQ 查表高速编解码器

> 参考来源：/data/dev/bun/src/sourcemap/VLQ.cp | 吸收日期：2026-11-22

`cap:vlq` 提供零堆分配、查表加速的 SourceMap v3 Base64-VLQ 编解码器：

- 编译期预计算 `0..=255` 快速查找表（`encode` 命中 O(1) 直接返回）与 128 字节 Base64 解码查找表；
- 内部采用 64 位无符号算术，完整支持全量 `i32` 范围（包含极限负值 `i32::MIN = -2147483648` 与 `i32::MAX = 2147483647`），零整数溢出风险；
- `decodeChecked` 严格拦截非 ASCII / 非 Base64 字符、截断续位流、越界偏移与 `i32` 溢出。

---

## 快速导入

```cp
import {
    VLQ,
    VLQResult,
    Error,
    ZERO,
    MAX_VLQ_BYTES,
    encode,
    encodeSlowPath,
    encodeInto,
    encodeSliceInto,
    decode,
    decodeChecked,
    decodeSegment,
} from "cap:vlq";
```

---

## API 参考

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `MAX_VLQ_BYTES` | `pub const MAX_VLQ_BYTES: usize = 7` | 单个 `i32` VLQ 编码最大字节数 |
| `ZERO` | `pub const ZERO: VLQ` | 预计算的 `0` 编码（`"A"`） |
| `VLQ` | `struct { bytes: [u8; 7], len: u8 }` | 栈上编码结果，提供 `slice()`、`copyTo(out)`、`writeTo(writer)` |
| `VLQResult` | `struct { value: i32, next: usize }` | 解码值与下一个字节偏移 |
| `Error` | `InvalidBase64Char`, `TruncatedInput`, `Overflow`, `BufferTooSmall`, `OutOfBounds` | 严格编解码错误集 |
| `encode` | `pub fn encode(value: i32): VLQ` | 0..=255 查表快速编码，其余走 `encodeSlowPath` |
| `encodeSlowPath` | `pub fn encodeSlowPath(value: i32): VLQ` | 通用无溢出编码（覆盖 `i32::MIN..=i32::MAX`） |
| `encodeInto` | `pub fn encodeInto(value: i32, out: []u8): usize!` | 编码单个整数并写入目标切片 |
| `encodeSliceInto` | `pub fn encodeSliceInto(values: []const i32, out: []u8): usize!` | 连续编码多个整数写入目标切片 |
| `decodeChecked` | `pub fn decodeChecked(encoded: string, start: usize): VLQResult!` | 严格校验解码 |
| `decode` | `pub fn decode(encoded: string, start: usize): VLQResult?` | 可选解码（非法输入返回 `null`） |
| `decodeSegment` | `pub fn decodeSegment(segment: string, out: []i32): usize!` | 连续解码单个 SourceMap 映射段中的全部字段 |

---

## 使用示例

```cp
import { encode, encodeSliceInto, decodeSegment } from "cap:vlq";

fn demo()! {
    const v = encode(16); // "gB"
    * = v.slice();

    let buf: [32]u8 = [0; 32];
    const fields: [4]i32 = [0, 0, 1, 4];
    const n = encodeSliceInto(fields[0..4], buf[0..32])?;

    let decoded: [4]i32 = [0; 4];
    const count = decodeSegment(buf[0..n], decoded[0..4])?;
    * = count; // 4
}
```

## 上游行为契约与数值边界防护

1. **热路径 O(1) 查表加速（源自 Bun `sourcemap/VLQ.cp`）**：
   - SourceMap 中超过 95% 的列/行增量落在 `0..=255` 区间，`encode` 直接命中编译期 256 项查找表零循环返回；超出范围自动无缝切至 `encodeSlowPath`。
2. **`i32::MIN` 无溢出与续位流安全**：
   - 内部先转 64 位无符号再做 ZigZag 符号位搬移，彻底避免对 `i32::MIN`（`-2147483648`）取反溢出；
   - `decodeChecked` 严格校验最高续位 bit 5 链长与边界，拦截截断流（`TruncatedInput`）、非法字符（`InvalidBase64Char`）与超 `i32` 范围编码（`Overflow`）。
