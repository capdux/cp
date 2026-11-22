# `cap:hamming` — Zero-Allocation Hamming(7,4), SECDED(8,4) & SECDED(72,64) ECC Library

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

Pure CP, zero-heap-allocation Error-Correcting Code (ECC) library providing classic Hamming(7,4), extended SECDED Hamming(8,4) (with byte-stream helpers), and server DRAM standard SECDED Hamming(72,64) single-error correction / double-error detection codecs.

## Types

- `EccStatus: u8`: `ok = 0`, `corrected = 1`, `double_error = 2`
- `Hamming4Decode`: `{ data: u8, corrected: bool, error_bit: u8 }` (`error_bit`: `0` = no error, `1..7` = corrected bit)
- `Secded4Decode`: `{ data: u8, status: EccStatus, error_bit: u8 }` (`error_bit`: `0` = no error/double error, `1..8` = corrected bit)
- `Secded64Decode`: `{ data: u64, ecc: u8, status: EccStatus, error_bit: u8 }` (`error_bit`: `0` = no error/double error, `1..72` = corrected bit)

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `hamming74Encode(nibble: u8): u8` | Encodes lower 4 bits into a 7-bit Hamming(7,4) codeword `[p1, p2, d1, p4, d2, d3, d4]`. |
| `hamming74Decode(code: u8): Hamming4Decode` | Decodes a 7-bit codeword and corrects any single-bit flip in positions `1..7`. |
| `secded84Encode(nibble: u8): u8` | Encodes lower 4 bits into an 8-bit extended SECDED codeword (bit 7 = overall even parity). |
| `secded84Decode(code: u8): Secded4Decode` | Corrects any single-bit flip (`1..8`) and detects all double-bit flips (`EccStatus.double_error`). |
| `secded84EncodeBytes(src: []const u8, dst: *[]u8): usize?` | Splits each byte into high/low nibbles and writes 2 SECDED(8,4) bytes per input byte. |
| `secded84DecodeBytes(src: []const u8, dst: *[]u8): usize?` | Decodes SECDED(8,4) stream, auto-correcting single-bit flips; returns `null` on odd length, short buffer, or any double-bit flip. |
| `secded7264Encode(data: u64): u8` | Generates an 8-bit server DRAM SECDED ECC byte (7 Hamming parity bits + 1 overall even parity bit). |
| `secded7264Decode(data: u64, ecc: u8): Secded64Decode` | Validates/corrects 64-bit data + 8-bit ECC; corrects all 72 single-bit flips (`1..72`) and detects all double-bit flips. |

## Example

```cp
import {
    EccStatus,
    secded7264Encode,
    secded7264Decode,
} from "cap:codec";

fn scrubWord(word: u64): u64 {
    const ecc = secded7264Encode(word);
    const corrupted = word ^ (1 as u64 << 19);
    const res = secded7264Decode(corrupted, ecc);
    if (res.status == EccStatus.corrected) {
        return res.data;
    }
    return word;
}
```
