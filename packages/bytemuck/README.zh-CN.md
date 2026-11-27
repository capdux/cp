[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bytemuck

适用于 CP 语言的零堆分配安全 POD（Plain Old Data）字节重解释、大小端整数/浮点转换、非对齐读取、对齐与长度布局校验及零拷贝切片视图库，对齐 Rust [`bytemuck`](https://github.com/Lokathor/bytemuck) crate（MIT 协议，CP v1.0.0）。

## 安装与导入

```bash
cap add bytemuck
```

```cp
import {
    POD_OK,
    POD_ERR_SIZE_MISMATCH,
    POD_ERR_ALIGNMENT,
    CastSliceResult,
    bytesOfU32,
    fromBytesU32,
    podReadUnalignedF64,
    tryCastBytesToU32Slice,
    castU32SliceAsBytes,
    tryCastSliceBytesToU32,
    swapBytesU32,
} from "bytemuck";
```

## 核心 API 参考

### 错误码与布局校验

| 符号 | 签名 / 取值 | 说明 |
| :--- | :--- | :--- |
| `POD_OK` / `POD_ERR_SIZE_MISMATCH` / `POD_ERR_ALIGNMENT` / `POD_ERR_OUTPUT_SLICE_TOO_SMALL` / `POD_ERR_TARGET_ALIGNMENT_GREATER` | `0` / `1` / `2` / `3` / `4` | `checkCastLayout` 与 `CastSliceResult.err_code` 返回的精确错误码。 |
| `CastSliceResult` | `count: usize`, `err_code: u8`, `isOk(): bool` | `tryCastBytesToU*Slice` 的结构化返回结果。 |
| `checkCastLayout` | `(byte_len, elem_size, addr, align_req): u8` | 校验字节长度整除性与内存地址对齐。 |
| `isAligned` | `(addr: usize, align_req: usize): bool` | 检查 `addr` 是否满足 `align_req` 对齐。 |
| `fillZeroed` / `zeroedU8..zeroedF64` | `([]u8)` / `(): T` | 清零字节切片或返回各基础类型零值（对齐 `Zeroable`）。 |

### 标量字节互转与非对齐读取

| 分组 | 函数 | 说明 |
| :--- | :--- | :--- |
| `bytesOf*` | `bytesOfU16`, `bytesOfU32`, `bytesOfU64`, `bytesOfF32`, `bytesOfF64` | 将标量 POD 值转为定长 `[N]u8`（小端序）。 |
| `fromBytes*` | `fromBytesU16`, `fromBytesU32`, `fromBytesU64`, `fromBytesF32`, `fromBytesF64` | 接收 `&[]u8`；要求长度严格等于 `sizeof(T)`，否则返回 `null`。 |
| `podReadUnaligned*` | `podReadUnalignedU16`, `U32`, `U64`, `I32`, `F32`, `F64` | 接收 `&[]u8`；只要 `buf.len >= sizeof(T)` 即可安全执行非对齐读取。 |
| 大小端读写 | `readU16Le/Be`, `writeU16Le/Be`, `readU32Le/Be`, `writeU32Le/Be`, `readU64Le/Be`, `writeU64Le/Be`, `readF32Le`, `writeF32Le`, `readF64Le`, `writeF64Le` | 显式大小端序读取（`T?`）与写入（`bool`）。 |
| 字节序翻转 | `swapBytesU16`, `swapBytesU32`, `swapBytesU64` | 翻转无符号整数字节序。 |

### 切片转换与零拷贝视图

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `castU16SliceAsBytes` / `U32` / `U64` | `(src: &[]u*): &[]u8` | 整数切片到只读字节切片 `&[]u8` 的零拷贝重解释。 |
| `tryCastSliceBytesToU32` | `(src: &[]u8): (&[]u32)?` | 对齐与长度严格校验的 `&[]u8` -> `&[]u32` 零拷贝视图。 |
| `tryCastBytesToU16Slice` / `U32` / `U64` | `(src: &[]u8, dst: []u*): CastSliceResult` | 安全小端解码至 `dst`，携带精确 `POD_*` 错误码。 |
| `castU16SliceToBytesLe` / `castBytesToU16SliceLe` | `(&[]u16, []u8): usize?` / `(&[]u8, []u16): usize?` | 返回可选计数的 `u16` 批量小端转换。 |
| `castU32SliceToBytesLe` / `castBytesToU32SliceLe` | `([]u32, []u8): usize?` / `([]u8, []u32): usize?` | `u32` 与字节切片批量小端转换。 |
| `castU64SliceToBytesLe` / `castBytesToU64SliceLe` | `(&[]u64, []u8): usize?` / `(&[]u8, []u64): usize?` | `u64` 与字节切片批量小端转换。 |

## 代码示例

### 1. 标量 `bytesOf*`、`fromBytes*` 与非对齐读取

```cp
import { bytesOfF32, fromBytesF32, podReadUnalignedU32 } from "bytemuck";

fn scalarCast() {
    let raw = bytesOfF32(3.5);
    let val = fromBytesF32(&raw); // 3.5

    let pkt: [6]u8 = [0xFF, 0x01, 0x02, 0x03, 0x04, 0xAA];
    let unaligned = podReadUnalignedU32(&pkt[1..6]); // 0x04030201
}
```

### 2. 零拷贝切片视图与带错误码的批量转换

```cp
import { castU32SliceAsBytes, tryCastBytesToU32Slice, POD_OK } from "bytemuck";

fn sliceCast() {
    let words: [2]u32 = [0x11223344, 0x55667788];
    let bytes = castU32SliceAsBytes(&words); // 零拷贝 8 字节 &[]u8 视图

    let out: [2]u32 = undefined;
    let res = tryCastBytesToU32Slice(bytes, out[0..2]);
    // res.isOk() == true, res.err_code == POD_OK, res.count == 2
}
```

## 与源包差异与零拷贝安全亮点

- **调用方无需编写不安全代码**：以预验证的强类型基础函数与切片转换替代 Rust `unsafe impl Pod`/`Zeroable`，底层依托 `cap:bin` 与显式对齐守卫。
- **任意偏移非对齐安全**：`podReadUnaligned*` 与 `tryCastBytesTo*Slice` 可在任意字节偏移安全解码，绝不触发硬件对齐异常。
- **零拷贝内存视图**：`castU16/U32/U64SliceAsBytes` 与 `tryCastSliceBytesToU32` 零拷贝转换内存视图并严格校验对齐与长度。
