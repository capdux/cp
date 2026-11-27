[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# zerocopy

面向 CP 的零堆内存、零拷贝字节切片前后缀解析器、有状态读写游标、显式字节序类型（`U16LE`/`U16BE`/`U32LE`/`U32BE`/`U64LE`/`U64BE`）及布局校验切片视图库。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/zerocopy](https://crates.io/crates/zerocopy) |
| **上游原包名** | `zerocopy`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/google/zerocopy |
| **许可证** | MIT（上游：BSD-2-Clause OR Apache-2.0 OR MIT） |

从 Rust 迁移到 CP 保持原包名不变，零成本直连导入：

```rust
// Rust
use zerocopy::{FromBytes, IntoBytes, byteorder::network_endian::U32};
```

```cp
// CP —— 保持原包名，零堆零拷贝原生性能
import {
    U32BE, ByteCursor, ByteWriterCursor,
    readU32BeFromPrefix, sliceAsBytesU32, refFromBytesSliceU32,
} from "zerocopy";
```

## 安装

```bash
cap add zerocopy
```

## 实战示例

### 1. 前后缀零拷贝切分与读写（`readFromPrefix` / `readFromSuffix` / `writeToPrefix`）

```cp
import {
    readFromPrefix, readFromSuffix,
    readU16BeFromPrefix, readU32BeFromPrefix, readU64LeFromSuffix,
    writeToPrefix, writeToSuffix,
} from "zerocopy";

fn parsePacket(pkt: &[]u8) {
    // 依次读取 2 字节大端协议头与 4 字节长度字段
    if (readU16BeFromPrefix(pkt)) |hdr| {
        let msg_type = hdr.value;
        if (readU32BeFromPrefix(hdr.rest)) |len_part| {
            let payload_len = len_part.value as usize;
            if (readFromPrefix(len_part.rest, payload_len)) |split| {
                let payload: &[]u8 = split.head;
                let trailer: &[]u8 = split.tail;
                if (readU64LeFromSuffix(trailer)) |crc| {
                    // crc.value 为尾部校验和，crc.rest 为剩余前缀字节
                }
            }
        }
    }

    // 写入目标缓冲区并返回剩余可写切片
    mut out: [16]u8 = [0; 16];
    let magic: [4]u8 = [0x43, 0x50, 0x00, 0x01];
    if (writeToPrefix(&mut out, &magic)) |rest| {
        let _ = writeToSuffix(rest, &magic);
    }
}
```

### 2. 有状态读写游标（`ByteCursor` & `ByteWriterCursor`）

```cp
import { ByteCursor, ByteWriterCursor } from "zerocopy";

fn demoCursors() {
    mut buf: [32]u8 = [0; 32];
    mut w = ByteWriterCursor.new(buf.len);
    let _ = w.putU8(&mut buf, 1);
    let _ = w.putU16Be(&mut buf, 8080);
    let _ = w.putU32Le(&mut buf, 0x12345678);
    let _ = w.putF64Le(&mut buf, 2.71828);
    let payload: [3]u8 = [65, 66, 67];
    let _ = w.putBytes(&mut buf, &payload);

    let total_written = w.written();
    mut r = ByteCursor.new(total_written);
    if (r.peekU8(&buf)) |first| {
        // 不推进游标的预览读取 == 1
    }
    let ver = r.nextU8(&mut buf);
    let port = r.nextU16Be(&mut buf);
    let id = r.nextU32Le(&mut buf);
    let ratio = r.nextF64Le(&buf);
    if (r.nextBytes(&buf, 3)) |tag| {
        // 直接借用 buf 中的 3 字节视图，零拷贝
    }
}
```

### 3. 显式字节序类型（`U16LE`..`U64BE`）与切片视图

```cp
import {
    U16LE, U16BE, U32LE, U32BE, U64LE, U64BE,
    refFromBytesU32, refFromBytesSliceU32,
    sliceAsBytesU16, sliceAsBytesU32, sliceAsBytesU64,
    isAlignedSlice,
} from "zerocopy";

fn demoEndianTypes() {
    mut net_port = U16BE.new(443);
    let wire_bytes: [2]u8 = net_port.toBytes();
    if (U16BE.fromBytes(&wire_bytes)) |parsed| {
        let port = parsed.get(); // 443
    }
    net_port.set(8443);

    // 零拷贝切片转换
    let nums: [2]u32 = [10, 20];
    let raw: &[]u8 = sliceAsBytesU32(&nums);
    if (refFromBytesSliceU32(raw)) |view| {
        // view.len == 2（未对齐或长度不整除时返回 null）
    }
    let ok = isAlignedSlice(raw.len, 4, 0x1000, 4);
}
```

## 性能与安全亮点

- **零堆分配与零拷贝**：`readFromPrefix`、`readFromSuffix`、`ByteCursor.nextBytes` 与 `sliceAsBytesU*` 直接返回借用子切片，不产生堆分配和内存拷贝。
- **越界安全游标**：`ByteCursor` 与 `ByteWriterCursor` 每次读写均自动检查 `remaining()`，不足时返回 `null` 或 `false`，杜绝越界访问。
- **无对齐依赖的显式字节序包装**：`U16LE`~`U64BE` 底层以 `[N]u8` 存储，不受宿主平台字节序与对齐限制。
- **严格布局校验**：复用 `bytemuck` 的对齐和尺寸校验逻辑，保障 `refFromBytesSliceU32` 等视图转换的内存安全。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
