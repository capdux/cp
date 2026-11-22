# `cap:base85` — Z85, Adobe Ascii85 与 RFC 1924 Base85 编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

> **模块路径**: `lib/base85/`  
> **导入协议**: `import { Z85, Ascii85, Rfc1924, Base85Error } from "cap:codec";`  
> **动态挂载**: `import cap from "cap";` -> `cap.base85.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:base85` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的 Base85 家族编解码引擎，统一覆盖三大工业标准：

1. **ZeroMQ Spec 32 Z85 (`Z85`)**：
   - 使用 Z85 标准字母表 `0-9a-zA-Z.-:+=^!/*?&<>()[]{}@%$#`。
   - 严格要求编码输入长度为 4 字节整数倍、解码输入长度为 5 字符整数倍（否则返回 `Base85Error.InvalidLength`）。
   - 解码时严格校验 32-bit 数值溢出（如 `"%%%%%"`，返回 `Base85Error.ValueOverflow`）与非法字符（`Base85Error.InvalidCharacter`）。
2. **Adobe Ascii85 (`Ascii85`)**：
   - 支持任意字节长度（尾部不足 4 字节时补零编码并按 `rem + 1` 截断输出）。
   - 4 字节全零块 `0x00000000` 自动压缩为 `'z'`；解码时若 `'z'` 出现在非块首位置则拦截为 `Base85Error.MalformedFraming`。
   - 提供无帧接口 `encode` / `decode` 与 `<~...~>` 帧封装接口 `encodeFramed` / `decodeFramed`，解码自动忽略 ASCII 空白符。
3. **RFC 1924 Base85 (`Rfc1924`)**：
   - 使用 RFC 1924 IPv6 紧凑字母表 `0-9A-Za-z!#$%&()*+-;<=>?@^_`{|}~`。
   - 支持任意字节长度的 4 字节块及尾部补零截断往返编解码，同样校验 32-bit 溢出与非法字符。

---

## 二、 核心 API 速查表 (`lib/base85/base85.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `Base85Error` | `InvalidLength`, `InvalidCharacter`, `ValueOverflow`, `MalformedFraming`, `OutOfMemory` |
| `Z85.encode(allocator, bytes)` | 将 4 字节对齐的字节切片编码为 Z85 字符串（调用方负责 `allocator.free`） |
| `Z85.decode(allocator, str)` | 将 5 字符对齐的 Z85 字符串解码为原始字节（严格校验溢出与非法字符） |
| `Ascii85.encode(allocator, bytes)` | 将任意长度字节编码为无 `<~...~>` 帧的 Adobe Ascii85（支持 `'z'` 全零压缩） |
| `Ascii85.decode(allocator, str)` | 解码无帧 Adobe Ascii85（忽略空白符，支持 `'z'` 与任意尾部块） |
| `Ascii85.encodeFramed(allocator, bytes)` | 将字节编码为带 `<~...~>` 帧的 Adobe Ascii85 |
| `Ascii85.decodeFramed(allocator, str)` | 解码带 `<~...~>` 帧的 Adobe Ascii85（缺失或错位帧返回 `MalformedFraming`） |
| `Rfc1924.encode(allocator, bytes)` | 使用 RFC 1924 字母表编码任意长度字节切片 |
| `Rfc1924.decode(allocator, str)` | 解码 RFC 1924 Base85 字符串为原始字节 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { Z85, Ascii85, Rfc1924 } from "cap:codec";

fn demo(alloc: std.mem.Allocator): !void {
    // 1. ZeroMQ Z85 (4 字节对齐)
    const raw = [_]u8{ 0x86, 0x4F, 0xD2, 0x6F, 0xB5, 0x59, 0xF7, 0x5B };
    const z85_str = Z85.encode(alloc, &raw)?; // "HelloWorld"
    defer alloc.free(z85_str);
    const z85_dec = Z85.decode(alloc, z85_str)?;
    defer alloc.free(z85_dec);

    // 2. Adobe Ascii85 (<~...~> 帧 + 'z' 全零压缩)
    const zeros = [_]u8{ 0, 0, 0, 0, 65 };
    const a85 = Ascii85.encodeFramed(alloc, &zeros)?;
    defer alloc.free(a85);
    const a85_dec = Ascii85.decodeFramed(alloc, a85)?;
    defer alloc.free(a85_dec);

    // 3. RFC 1924 Base85
    const rfc = Rfc1924.encode(alloc, "CP-Lang")?;
    defer alloc.free(rfc);
    const rfc_dec = Rfc1924.decode(alloc, rfc)?;
    defer alloc.free(rfc_dec);
}
```
