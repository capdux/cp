# `cap:xdr` — RFC 4506 XDR (External Data Representation) 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**: `lib/xdr/`  
> **导入协议**: `import { XDR_UNIT, xdrPadLen, xdrAlignedLen, XdrReader, XdrWriter } from "cap:binfmt";`  
> **动态挂载**: `import cap from "cap";` -> `cap.xdr.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:xdr` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 4506 XDR（External Data Representation Standard，广泛用于 ONC RPC、NFS v3/v4、Stellar 等协议）大端 4 字节对齐编解码库：

1. **全套标量流式编解码**：
   - 完整支持 `i32` / `u32` / `i64` / `u64` / `bool` / `f32`（IEEE 754 单精度）/ `f64`（IEEE 754 双精度）。
   - `readBool` 严格遵循 RFC 4506，仅接受 `0` 或 `1`，拒绝其他非规范布尔值。
2. **定长与变长 Opaque / String / Array 4B 对齐**：
   - `writeFixedOpaque` / `writeOpaque` / `writeString` 自动补齐 `0..3` 字节 `0x00` 填充至 4 字节边界。
   - `readFixedOpaque` / `readOpaque` / `readString` 零拷贝返回数据切片，并严格校验所有尾部填充字节必须为 `0x00`。
3. **原子边界与溢出防护**：
   - 读写长度校验全程防整数溢出；缓冲区不足、截断或非法填充时原子失败不破坏游标。

---

## 二、 核心 API 速查表 (`lib/xdr/xdr.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 常量与对齐 | `XDR_UNIT (4)`, `xdrPadLen(len): usize`, `xdrAlignedLen(len): usize` |
| `XdrReader` | `init(data)`, `remaining()`, `isEof()`, `skip(bytes)`, `readI32()`, `readU32()`, `readI64()`, `readU64()`, `readBool()`, `readF32()`, `readF64()`, `readFixedOpaque(len)`, `readOpaque()`, `readString()`, `readArrayLen()` |
| `XdrWriter` | `init(buf)`, `written()`, `remaining()`, `writeI32(v)`, `writeU32(v)`, `writeI64(v)`, `writeU64(v)`, `writeBool(v)`, `writeF32(v)`, `writeF64(v)`, `writeFixedOpaque(bytes)`, `writeOpaque(bytes)`, `writeString(s)`, `writeArrayLen(n)` |

---

## 三、 使用示例

```typescript
import { XdrWriter, XdrReader } from "cap:binfmt";

fn demo() {
    let buf: [128]u8 = [0; 128];
    let w = XdrWriter.init(buf[0..]);
    * = w.writeU32(0x12345678);
    * = w.writeBool(true);
    * = w.writeString("nfs-v4");

    let r = XdrReader.init(w.written());
    const xid = r.readU32();
    const ok = r.readBool();
    const name = r.readString();
    * = xid;
    * = ok;
    * = name;
}
```
