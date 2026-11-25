# `@cap/bytes`

工业级、纯 CP 实现、零外部依赖、零堆分配的字节大小解析与人类可读格式化库（Clean-Room 自主实现），对标 npm `bytes` 与 Rust `bytesize` / `humansize` crate。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无需 C/FFI、无 `unsafe`、无堆分配，全程基于调用方栈缓冲区与纯整数（`u64` / `u128`）精确运算。
- **双轨单位体系**：同时完整支持二进制 IEC（`KiB`/`MiB`/`GiB`/`TiB`/`PiB`，1024 基数）与十进制 SI（`KB`/`MB`/`GB`/`TB`/`PB`，1000 基数）。
- **高精度无误差解析**：纯整数解析整数与小数部分，彻底避免 IEEE-754 浮点精度丢失，并严格拦截一切 `u64` 乘法/加法溢出。
- **智能格式化**：支持 0..4 位小数配置、空格开关、多余尾零自动裁剪、四舍五入跨单位进位（如 `1023.999 KiB` -> `1 MiB`）及小缓冲区越界防护。

## 常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | ---: | :--- |
| `KIB` | `u64` | `1024` | $1024^1$ 字节（IEC 二进制） |
| `MIB` | `u64` | `1048576` | $1024^2$ 字节（IEC 二进制） |
| `GIB` | `u64` | `1073741824` | $1024^3$ 字节（IEC 二进制） |
| `TIB` | `u64` | `1099511627776` | $1024^4$ 字节（IEC 二进制） |
| `PIB` | `u64` | `1125899906842624` | $1024^5$ 字节（IEC 二进制） |
| `KB` | `u64` | `1000` | $1000^1$ 字节（SI 十进制） |
| `MB` | `u64` | `1000000` | $1000^2$ 字节（SI 十进制） |
| `GB` | `u64` | `1000000000` | $1000^3$ 字节（SI 十进制） |
| `TB` | `u64` | `1000000000000` | $1000^4$ 字节（SI 十进制） |
| `PB` | `u64` | `1000000000000000` | $1000^5$ 字节（SI 十进制） |

## 类型与错误定义

### `BytesError`

```typescript
pub err BytesError {
    EmptyInput,     // 输入为空或仅包含空白字符
    InvalidFormat,  // 数值格式非法（如负数、多小数点、无数字）
    UnknownUnit,    // 无法识别的单位后缀
    Overflow,       // 解析结果超出 u64 上限 (18446744073709551615)
    BufferTooSmall, // 输出缓冲区容量不足
}
```

### `ByteUnitMode` 与 `FormatOptions`

```typescript
pub enum ByteUnitMode {
    BinaryIec, // 1024 基数: B, KiB, MiB, GiB, TiB, PiB
    DecimalSi, // 1000 基数: B, KB, MB, GB, TB, PB
}

pub struct FormatOptions {
    mode: ByteUnitMode = ByteUnitMode.BinaryIec;
    decimals: u8 = 2;
    space: bool = true;
}
```

## API 文档

### 解析函数

- **`pub fn parseBytes(input: []const u8): u64 ! BytesError`**  
  按二进制 1024 基数解析字节字符串（兼容 npm `bytes` 约定），等价于 `parseBytesBinary`。支持纯数字、小数、大小写不敏感、首尾与中间可选空格，支持 `B`/`Byte`/`Bytes`、`K`/`KB`/`KiB`、`M`/`MB`/`MiB`、`G`/`GB`/`GiB`、`T`/`TB`/`TiB`、`P`/`PB`/`PiB`。

- **`pub fn parseBytesBinary(input: []const u8): u64 ! BytesError`**  
  显式使用二进制（1024 基数）解析所有标准与 IEC 单位后缀。

- **`pub fn parseBytesSi(input: []const u8): u64 ! BytesError`**  
  使用十进制 SI（1000 基数）解析 `K/KB/M/MB/G/GB/T/TB/P/PB`，同时显式 IEC 后缀（`KiB/MiB/GiB/TiB/PiB`）仍保持 1024 基数。

### 格式化函数

- **`pub fn formatBytes(n: u64, opts: FormatOptions, out: []u8): ([]const u8) ! BytesError`**  
  按 `opts` 将字节数 `n` 格式化写入调用方提供的 `out` 缓冲区，返回实际写入的只读切片；缓冲区不足时返回 `BytesError.BufferTooSmall`。

- **`pub fn formatBinary(n: u64, out: []u8): ([]const u8) ! BytesError`**  
  快捷二进制 IEC 格式化（`mode = BinaryIec`, `decimals = 2`, `space = true`），如 `"1.5 MiB"`。

- **`pub fn formatDecimal(n: u64, out: []u8): ([]const u8) ! BytesError`**  
  快捷十进制 SI 格式化（`mode = DecimalSi`, `decimals = 2`, `space = true`），如 `"1.5 MB"`。

- **`pub fn formatCompact(n: u64, out: []u8): ([]const u8) ! BytesError`**  
  快捷无空格短格式（`mode = DecimalSi`, `decimals = 2`, `space = false`），如 `"1.5MB"`、`"512B"`。

## 使用示例

### 示例 1：解析配置中的内存与限流大小（Binary vs SI）

```typescript
import {
    parseBytes,
    parseBytesSi,
    KIB,
    MIB,
    GB,
} from "@cap/bytes";
import { print } from "cap:io";

pub fn main() {
    // 默认二进制基数（兼容 npm `bytes`）
    const limit = parseBytes("1.5MB") catch unreachable;        // 1572864 (1.5 * 1024^2)
    const cache = parseBytes("2.5 GiB") catch unreachable;      // 2684354560
    const tiny = parseBytes("512 b") catch unreachable;         // 512

    // SI 十进制基数（显式 IEC 后缀仍保持 1024）
    const disk = parseBytesSi("4TB") catch unreachable;         // 4000000000000
    const ram = parseBytesSi("16 KiB") catch unreachable;       // 16384

    if (limit == MIB + 512 * KIB && disk == 4000 * GB && tiny == 512 && ram == 16 * KIB) {
        print("Parsed byte sizes successfully");
    }
    * = cache;
}
```

### 示例 2：格式化为二进制 IEC、十进制 SI 与紧凑格式

```typescript
import {
    formatBinary,
    formatDecimal,
    formatCompact,
    MIB,
    MB,
} from "@cap/bytes";
import { print } from "cap:io";

pub fn main() {
    let buf1: [32]u8 = [0; 32];
    let buf2: [32]u8 = [0; 32];
    let buf3: [32]u8 = [0; 32];

    const s_bin = formatBinary(MIB + 524288, buf1[0..]) catch unreachable;   // "1.5 MiB"
    const s_dec = formatDecimal(MB + 500000, buf2[0..]) catch unreachable;   // "1.5 MB"
    const s_cmp = formatCompact(MB + 500000, buf3[0..]) catch unreachable;   // "1.5MB"

    print(s_bin);
    print(s_dec);
    print(s_cmp);
}
```

### 示例 3：自定义小数位与错误处理（溢出 / 小缓冲区防护）

```typescript
import {
    ByteUnitMode,
    FormatOptions,
    BytesError,
    parseBytes,
    formatBytes,
    GIB,
} from "@cap/bytes";
import { print } from "cap:io";

pub fn main() {
    let out: [32]u8 = [0; 32];
    const opts = FormatOptions {
        mode: ByteUnitMode.BinaryIec,
        decimals: 3,
        space: true,
    };
    const text = formatBytes(GIB + 123456789, opts, out[0..]) catch unreachable;
    print(text);

    // 捕获溢出错误
    * = parseBytes("99999999999999999999999") catch e => {
        if (e == BytesError.Overflow) {
            print("Overflow safely caught");
        }
    };

    // 捕获输出缓冲区不足错误
    let tiny_buf: [2]u8 = [0; 2];
    * = formatBytes(GIB, FormatOptions { mode: ByteUnitMode.BinaryIec, decimals: 2, space: true }, tiny_buf[0..]) catch e => {
        if (e == BytesError.BufferTooSmall) {
            print("BufferTooSmall safely caught");
        }
    };
}
```

## License

Copyright (c) 2026 CP Contributors.

Licensed under the **MIT License**.
