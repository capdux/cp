# CP 标准库：libpcap 容器与 L2-L4 网络协议解包器 (`cap:pcap`)

## 一、 架构总览与设计准则

`cap:pcap` 是 CP 语言官方内置的纯 CP **`.pcap` 容器编解码器与轻量无拷贝 L2–L4 网络协议分层解包器**。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，完全基于内存字节流运行，不依赖底层网卡抓包驱动或 libpcap C 库，同时支持 `import { ... } from "cap:pcap"` 与 `import cap from "cap"; cap.pcap.*` 动态懒挂载调用。

### 核心特性

- **完整 `.pcap` 全局头与包记录编解码**：
  - 支持 Little-Endian 与 Big-Endian 两种字节序，以及 Microsecond (`0xa1b2c3d4` / `0xd4c3b2a1`) 与 Nanosecond (`0xa1b23c4d` / `0x4d3cb2a1`) 两种时间戳精度魔数；
  - 严格校验 `2.4` 版本号、`incl_len <= snaplen` 与 `max_snaplen` 上限防护（`PacketTooLarge`）以及全局头/包头/包体截断拦截（`TruncatedHeader` / `TruncatedPacket`）。
- **零拷贝 L2 -> L3 -> L4 分层解包器**：
  - **L2 Ethernet II (`parseEthernet`)**：解析 `dst_mac`、`src_mac`、`ether_type` 与 `payload`，自动跳过单层或多层 802.1Q (`0x8100`) / QinQ (`0x88a8`) VLAN 标签；
  - **L3 IPv4 (`parseIpv4`)**：校验 `version == 4`、`ihl >= 20`、`total_len >= ihl`，提取 `ttl`、`protocol`、`src_ip`、`dst_ip` 与精确长度的 `payload`；
  - **L4 TCP (`parseTcp`)**：解析源/目的端口、`seq`、`ack`、`data_offset`（支持 TCP 选项跳过）、`flags`、`window` 与 `payload`；
  - **L4 UDP (`parseUdp`)**：解析源/目的端口、`length`（校验 `>= 8`）与 `payload`。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pcap/pcap.cp` | `// Source: cp/lib/pcap/pcap.cp (...)` | `PcapError`、`PcapWriter`、`PcapFile`、`parsePcap`、`parsePcapWithMaxSnaplen`、`parseEthernet`、`parseIpv4`、`parseTcp`、`parseUdp` |
| `lib/pcap/index.cp` | `// Source: cp/lib/pcap/index.cp (...)` | `cap:pcap` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础枚举

- **`PcapError`**：`InvalidMagic`、`UnsupportedVersion`、`TruncatedHeader`、`TruncatedPacket`、`PacketTooLarge`、`InvalidEthernet`、`InvalidIpv4`、`InvalidTransport`、`OutOfMemory`
- **`PcapEndian`**：`Little`、`Big`
- **`PcapPrecision`**：`Microsecond` (`0xa1b2c3d4`)、`Nanosecond` (`0xa1b23c4d`)
- **`LinkType: u32`**：`Ethernet = 1`、`RawIp = 101`、`LinuxSll = 113`

### 2. `.pcap` 容器读写 (`PcapWriter` / `parsePcap`)

- **`PcapWriter`**：
  - `init(allocator, endian, precision, snaplen, link_type): PcapError!PcapWriter`
  - `addPacket(ts_sec, ts_subsec, orig_len, data): PcapError!void`
  - `finish(allocator): PcapError![]u8`
  - `deinit()`
- **`parsePcap(allocator, bytes): PcapError!PcapFile`**
- **`parsePcapWithMaxSnaplen(allocator, bytes, max_snaplen): PcapError!PcapFile`**
  - 返回 `PcapFile { header: PcapGlobalHeader, packets: []PcapPacket }`；其中 `PcapPacket.data` 为指向输入 `bytes` 的零拷贝切片，使用完毕后调用 `file.deinit(allocator)` 释放 `packets` 切片数组。

### 3. L2–L4 网络协议分层解包

- **`parseEthernet(frame): PcapError!EthernetFrame`**
  - 返回 `{ dst_mac: [6]u8, src_mac: [6]u8, ether_type: u16, payload: []const u8 }`
- **`parseIpv4(bytes): PcapError!Ipv4Packet`**
  - 返回 `{ ihl: u8, protocol: u8, ttl: u8, total_len: u16, src_ip: [4]u8, dst_ip: [4]u8, payload: []const u8 }`
- **`parseTcp(bytes): PcapError!TcpSegment`**
  - 返回 `{ src_port: u16, dst_port: u16, seq: u32, ack: u32, data_offset: u8, flags: u8, window: u16, payload: []const u8 }`
- **`parseUdp(bytes): PcapError!UdpDatagram`**
  - 返回 `{ src_port: u16, dst_port: u16, length: u16, payload: []const u8 }`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { PcapWriter, PcapEndian, PcapPrecision, LinkType, parsePcap, parseEthernet, parseIpv4, parseTcp } from "cap:pcap";
import cap from "cap";

pub fn demoPcap()! {
    const alloc = std.heap.page_allocator;
    let writer = PcapWriter.init(alloc, .Little, .Microsecond, 65535, @intFromEnum(LinkType.Ethernet))?;
    defer writer.deinit();

    // 写入原始以太网帧后生成 .pcap 字节流
    const raw_bytes = writer.finish(alloc)?;
    defer alloc.free(raw_bytes);

    let file = cap.pcap.parsePcap(alloc, raw_bytes)?;
    defer file.deinit(alloc);

    if (file.packets.len > 0) {
        const eth = parseEthernet(file.packets[0].data)?;
        if (eth.ether_type == 0x0800) {
            const ip = parseIpv4(eth.payload)?;
            if (ip.protocol == 6) {
                const tcp = parseTcp(ip.payload)?;
                _ = tcp;
            }
        }
    }
}
```
