# `cap:llmnr` — RFC 4795 LLMNR 链路本地组播名称解析零分配编解码库

> **模块路径**: `lib/llmnr/`  
> **导入协议**: `import { LLMNR_PORT, LLMNR_MULTICAST_V4, LLMNR_DEFAULT_TTL, TYPE_A, TYPE_AAAA, parseLlmnr, encodeLlmnrQuery, encodeLlmnrResponseA, encodeLlmnrResponseAaaa } from "cap:llmnr";`  
> **动态挂载**: `import cap from "cap";` -> `cap.llmnr.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:llmnr` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 4795（Link-Local Multicast Name Resolution）线协议编解码库：

1. **严格 RFC 4795 校验 (`isLlmnr` / `parseLlmnr`)**：
   - 强制校验 `len >= 12`、`OPCODE == 0`。
   - 对查询报文（`QR == 0`）严格校验 `QDCOUNT == 1 && ANCOUNT == 0 && NSCOUNT == 0` 以及 `C == 0, TC == 0, T == 0, RCODE == 0`。
2. **零拷贝报文视图与安全压缩指针解码 (`LlmnrPacket` / `LlmnrRecordIter`)**：
   - `decodeName(offset, out)` 完整支持 `0xC0` DNS 压缩指针，并内置最大跳数与自引用防环保护。
   - `LlmnrRecord` 提供 `asIpv4()` 与 `asIpv6()` 便捷提取。
3. **零分配查询与响应编码 (`encodeLlmnrQuery` / `encodeLlmnrResponseA` / `encodeLlmnrResponseAaaa`)**：
   - 响应自动使用 `0xC00C` 压缩指针复用 Question 域名，紧凑写入调用方缓冲区。

---

## 二、 核心 API 速查表 (`lib/llmnr/llmnr.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `LLMNR_PORT (5355)`, `LLMNR_MULTICAST_V4 ("224.0.0.252")`, `LLMNR_MULTICAST_V6 ("ff02::1:3")`, `LLMNR_HEADER_SIZE (12)`, `LLMNR_DEFAULT_TTL (30)`, `CLASS_IN (1)`, `TYPE_A (1)`, `TYPE_PTR (12)`, `TYPE_AAAA (28)`, `TYPE_ANY (255)` |
| 标志常量 | `FLAG_QR (0x8000)`, `FLAG_CONFLICT (0x0400)`, `FLAG_TC (0x0200)`, `FLAG_TENTATIVE (0x0100)` |
| `LlmnrHeader` | `{ id, flags, qd_count, an_count, ns_count, ar_count }`；含 `isResponse()`, `opcode()`, `isConflict()`, `isTruncated()`, `isTentative()`, `rcode()` |
| `LlmnrQuestion` / `LlmnrRecord` | Question 与零拷贝 RR（含 `asIpv4()`, `asIpv6()`）及 `LlmnrRecordIter` |
| `LlmnrPacket` | `{ raw, header }`；含 `decodeName(offset, out)`, `question()`, `answers()` |
| 解析与编码 | `isLlmnr`, `parseLlmnr`, `encodeLlmnrQuery`, `encodeLlmnrResponseA`, `encodeLlmnrResponseAaaa` |

---

## 三、 使用示例

```typescript
import { TYPE_A, LLMNR_DEFAULT_TTL, encodeLlmnrResponseA, parseLlmnr } from "cap:llmnr";

fn demo() {
    let wire: [128]u8 = [0; 128];
    const ip: [4]u8 = [192, 168, 1, 42];
    if (n := encodeLlmnrResponseA(0x1234, "workstation", ip, LLMNR_DEFAULT_TTL, wire[0..])) {
        if (pkt := parseLlmnr(wire[0..n])) {
            let it = pkt.answers();
            if (rr := it.next()) {
                * = rr.asIpv4();
            }
        }
    }
}
```
