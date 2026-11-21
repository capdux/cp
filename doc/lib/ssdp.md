# `cap:ssdp` — UPnP / DLNA SSDP 局域网服务发现零分配编解码库

> **模块路径**: `lib/ssdp/`  
> **导入协议**: `import { SSDP_UDP_PORT, SSDP_MULTICAST_V4, SSDP_ST_ALL, SSDP_NTS_ALIVE, SsdpKind, parseSsdp, encodeSsdpMSearch, encodeSsdpNotify } from "cap:ssdp";`  
> **动态挂载**: `import cap from "cap";` -> `cap.ssdp.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:ssdp` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 UPnP / DLNA SSDP（Simple Service Discovery Protocol）编解码库：

1. **三类起始行解析 (`isSsdp` / `parseSsdp`)**：
   - 完整支持 `M-SEARCH * HTTP/1.1`、`NOTIFY * HTTP/1.1` 与 `HTTP/1.1 200 OK` 响应（含 3 位状态码提取）。
   - 严格拒绝空报文、缺失行尾或未知 HTTP 方法。
2. **零拷贝 HTTP-like 头部迭代与大小写无关查找 (`SsdpHeaderIter` / `SsdpMessage`)**：
   - 遇到空行（`\r\n\r\n` 或 `\n\n`）自动终止，支持任意大小写查找 `ST`/`NT`/`NTS`/`USN`/`LOCATION`/`SERVER`/`MAN`/`MX`。
   - 内置 `maxAge()` 从 `CACHE-CONTROL: max-age=1800`（兼容多指令逗号分隔）解析 TTL 秒数，以及 `isAlive()` / `isByebye()` 语义判定。
3. **防注入零分配编码 (`encodeSsdpMSearch` / `encodeSsdpNotify`)**：
   - 直接写入调用方缓冲区，自动拦截 `\r`/`\n`/`\0` 头部注入与缓冲区溢出。

---

## 二、 核心 API 速查表 (`lib/ssdp/ssdp.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `SSDP_UDP_PORT (1900)`, `SSDP_MULTICAST_V4 ("239.255.255.250")`, `SSDP_MULTICAST_V6_LINK ("ff02::c")`, `SSDP_MAN_DISCOVER`, `SSDP_ST_ALL`, `SSDP_ST_ROOTDEVICE`, `SSDP_NTS_ALIVE`, `SSDP_NTS_BYEBYE`, `SSDP_NTS_UPDATE` |
| `SsdpKind` | `m_search = 1`, `notify = 2`, `response = 3` |
| `SsdpHeader` / `SsdpHeaderIter` | `{ name, value }` 零拷贝头部迭代器（遇空行终止） |
| `SsdpMessage` | `{ kind, status_code, headers_bytes }`；含 `headers()`, `findHeader(name)`, `st()`, `nt()`, `nts()`, `usn()`, `location()`, `server()`, `man()`, `mx()`, `maxAge()`, `isAlive()`, `isByebye()` |
| 解析与编码 | `isSsdp(data)`, `parseSsdp(data)`, `encodeSsdpMSearch(st, mx, out)`, `encodeSsdpNotify(nt, nts, usn, location, max_age, out)` |

---

## 三、 使用示例

```typescript
import { SSDP_ST_ROOTDEVICE, encodeSsdpMSearch, parseSsdp } from "cap:ssdp";

fn demo() {
    let buf: [256]u8 = [0; 256];
    if (n := encodeSsdpMSearch(SSDP_ST_ROOTDEVICE, 3, buf[0..])) {
        if (msg := parseSsdp(buf[0..n])) {
            // msg.kind == SsdpKind.m_search, msg.mx() == 3
            * = msg;
        }
    }
}
```
