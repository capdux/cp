# `cap:golay` — 扩展二元戈莱码 [24, 12, 8] & 完美戈莱码 [23, 12, 7]

纯 CP、零堆分配的二元戈莱前向纠错码（FEC）标准库，采用自对偶系统生成矩阵 `G = [I_12 | B]` 与经典 4 步 IMLD 伴随式解析译码器，支持任意 3 比特纠错与 4 比特检错（旅行者号深空通信同款编码）。

## 导入

```cp
import {
    GOLAY24_DATA_BITS,
    GOLAY24_CODE_BITS,
    GOLAY24_MIN_DIST,
    GOLAY23_CODE_BITS,
    GolayStatus,
    GolayDecode,
    golay24Parity,
    golay24Encode,
    golay24Decode,
    golay23Encode,
    golay23Decode,
    golay24EncodeBytes,
    golay24DecodeBytes,
} from "cap:golay";
```

## 核心 API

- **协议常量与类型**：
  - `GOLAY24_DATA_BITS = 12`、`GOLAY24_CODE_BITS = 24`、`GOLAY24_MIN_DIST = 8`、`GOLAY23_CODE_BITS = 23`
  - `GolayStatus: u8` — `ok = 0`、`corrected = 1`、`uncorrectable = 2`
  - `GolayDecode` — `{ data: u16, codeword: u32, errors_corrected: u8, status: GolayStatus }`
- **单码字编解码**：
  - `golay24Parity(data12: u16): u16` — 计算 12-bit 校验位。
  - `golay24Encode(data12: u16): u32` — 编码为 24-bit 系统码字 `(data12 << 12) | parity12`。
  - `golay24Decode(codeword24: u32): GolayDecode` — 4 步 IMLD 译码，精确纠正 0/1/2/3 比特错误，检出 4 比特不可纠错误。
  - `golay23Encode(data12: u16): u32` — 穿孔生成完美 [23, 12, 7] 码字。
  - `golay23Decode(codeword23: u32): GolayDecode` — 奇偶补齐回落至 [24, 12, 8]，唯一纠正任意 <= 3 比特错误。
- **字节流分块编解码**：
  - `golay24EncodeBytes(src: []const u8, dst: []u8): usize?` — 每 3 字节拆 2 个 12-bit 编码为 6 字节（余 1 字节输出 3 字节；余 2 字节补 0 输出 6 字节）。
  - `golay24DecodeBytes(src: []const u8, dst: []u8): usize?` — 解码并纠正每个 24-bit 码字中至多 3 比特错误，遇不可纠错误或长度不合法返回 `null`。

## 示例

```cp
import { golay24Encode, golay24Decode, GolayStatus } from "cap:golay";

fn verify3BitRecovery(payload: u16): bool {
    const cw = golay24Encode(payload);
    // Flip 3 arbitrary bits across data & parity
    const corrupted = cw ^ ((1 as u32) << 2) ^ ((1 as u32) << 11) ^ ((1 as u32) << 19);
    const res = golay24Decode(corrupted);
    return res.status == GolayStatus.corrected && res.errors_corrected == 3 && res.data == (payload & 0x0FFF);
}
```
