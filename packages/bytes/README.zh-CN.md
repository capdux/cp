[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `bytes`

面向 CP 语言的零堆分配字节大小解析/格式化库（支持 IEC `KiB`/`MiB`/`GiB` 与 SI `KB`/`MB`/`GB`）与零拷贝二进制缓冲游标（`BytesReader` / `BytesMut`）——同时融合 **npm `bytes`** 与 **crates.io `bytes`** 双生态超集。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:bytes` ([visionmedia/bytes.js](https://github.com/visionmedia/bytes.js), v3 对齐) & `crates.io:bytes` ([tokio-rs/bytes](https://github.com/tokio-rs/bytes), v1 对齐) |
| **CP 包名与版本** | `bytes@1.0.0` (`MIT`) |
| **外部依赖** | 无（仅依赖内置 `cap:bin`、`cap:fmt`、`cap:ascii`） |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm bytes)
import bytes from "bytes";
const n = bytes.parse("1.5MB"); // 1572864
```

```rust
// Rust (crates.io bytes)
use bytes::{Buf, BufMut, BytesMut};
let mut buf = BytesMut::new();
buf.put_u32(0x12345678);
```

```cp
// CP — 保持原包名，无缝融合双生态 API
import { parse, formatIec, BytesMut, BytesReader } from "bytes";

const sz = parse("1.5MB"); // 1572864
let bm = BytesMut.new();
_ = bm.putU32Be(0x12345678);
```

## 架构特性与零堆设计

- **零动态内存分配**：`BytesReader` 直接借用 `&[]u8` 执行零拷贝游标读取；`BytesMut` 内置 512 字节栈缓冲区（`BYTES_MUT_CAP = 512`）。
- **端序安全的二进制编解码**：底层直连 `cap:bin` 字节序原语，在 `ReleaseFast` 下编译为原生 LLVM 字节序指令。
- **IEC / SI 双单位体系**：内置 `KIB`..`PIB`（1024 进制）与 `KB`..`PB`（1000 进制）常量及高精度解析/格式化函数。

## 实战示例

### 1. 人类可读字节大小解析与格式化

```cp
import { parse, parseBytes, formatBytes, formatIec, formatSi } from "bytes";

fn demoByteSizes() {
    const a = parse("1.5MB");              // 1572864（默认二进制单位）
    const b = parseBytes("1.5MB", false);  // 1500000（SI 十进制单位）
    const bad = parse("invalid");          // 解析失败返回 -1

    let buf: [64]u8 = undefined;
    const n1 = formatIec(1572864, buf[0..]);         // "1.5 MiB"
    const n2 = formatSi(1500000, buf[0..]);          // "1.5 MB"
    const n3 = formatBytes(1536, true, 2, buf[0..]); // "1.50 KiB"
    _ = a; _ = b; _ = bad; _ = n1; _ = n2; _ = n3;
}
```

### 2. 使用 `BytesMut` 与 `BytesReader` 进行零拷贝二进制封包/解包

```cp
import { BytesMut, BytesReader } from "bytes";

fn demoBinaryCodec() {
    // 栈上编码协议头与载荷（零堆分配）
    let w = BytesMut.new();
    if (!w.putU16Be(0x0102)) { return; }
    if (!w.putU32Le(42)) { return; }
    if (!w.putSlice("hello")) { return; }

    let wire: [64]u8 = undefined;
    const total = w.copyAll(wire[0..]);

    // 带边界校验的零拷贝游标解码
    let r = BytesReader.new(wire[0..total]);
    const magic = r.getU16Be();
    const seq = r.getU32Le();
    if (!magic.ok || !seq.ok) { return; }

    let payload: [5]u8 = undefined;
    if (!r.copyToSlice(payload[0..])) { return; }
    // magic.val == 0x0102, seq.val == 42, payload == "hello"
}
```

### 3. 进阶边界场景：`splitTo` 切分、容量溢出与截断安全

```cp
import { BytesMut, BytesReader } from "bytes";

fn demoEdgeCases() {
    let w = BytesMut.new();
    _ = w.putSlice("header:body");

    // 切分前缀到调用方缓冲区（若 at > used 或 out 空间不足则返回 -1）
    let head: [16]u8 = undefined;
    const split_len = w.splitTo(7, head[0..]); // 7 ("header:")，w 中保留 "body"

    // 字节不足时安全返回 ok == false，不会越界
    let r = BytesReader.new(head[0..2]);
    const u64_res = r.getU64Be(); // u64_res.ok == false
    _ = split_len; _ = u64_res;
}
```

## 核心 API 速览

- **大小解析/格式化**：`parse`、`parseBytes`、`formatBytes`、`formatIec`、`formatSi`、`KIB`..`PIB`、`KB`..`PB`
- **二进制读取游标**：`BytesReader`（`new`、`remaining`、`hasRemaining`、`position`、`advance`、`getU8`/`getU16Be`/`Le`/`getU32Be`/`Le`/`getU64Be`/`Le`、`copyToSlice`）
- **二进制写入缓冲**：`BytesMut`（`new`、`len`、`capacity`、`remainingMut`、`isEmpty`、`clear`、`truncate`、`byteAt`、`putU8`/`putU16Be`/`Le`/`putU32Be`/`Le`/`putU64Be`/`Le`、`putSlice`、`splitTo`、`copyAll`）

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
