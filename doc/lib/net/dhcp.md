# `cap:dhcp` — 零分配 RFC 2131 / RFC 2132 DHCPv4 线协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**：`lib/dhcp/`（入口 `lib/dhcp/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 RFC 2131 / 2132 DHCPv4 240 字节报头与 Magic Cookie (`0x63825363`) 校验、自动跳过 `OPT_PAD (0)` 并于 `OPT_END (255)` 截停的 Option 迭代器、常用 Option 强类型提取以及零分配 `DhcpWriter` 编码器。

---

## 一、导入方式

```cp
import {
    DHCP_SERVER_PORT,
    DHCP_CLIENT_PORT,
    DHCP_FIXED_HEADER_SIZE,
    DHCP_MIN_PACKET_SIZE,
    DHCP_MAGIC_COOKIE,
    DHCP_OP_REQUEST,
    DHCP_OP_REPLY,
    DHCP_FLAG_BROADCAST,
    OPT_PAD,
    OPT_SUBNET_MASK,
    OPT_ROUTER,
    OPT_DNS,
    OPT_HOSTNAME,
    OPT_DOMAIN_NAME,
    OPT_REQUESTED_IP,
    OPT_LEASE_TIME,
    OPT_MSG_TYPE,
    OPT_SERVER_ID,
    OPT_PARAM_REQ_LIST,
    OPT_MAX_MSG_SIZE,
    OPT_RENEWAL_TIME,
    OPT_REBINDING_TIME,
    OPT_VENDOR_CLASS,
    OPT_CLIENT_ID,
    OPT_END,
    DhcpMsgType,
    DhcpHeader,
    DhcpOption,
    DhcpOptIter,
    DhcpPacket,
    DhcpWriter,
    parseDhcp,
    isDhcp,
} from "cap:net";
```

---

## 二、核心数据结构

| 符号 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `DhcpMsgType: u8` | `discover (1)` ~ `inform (8)`, `unknown (255)` | Option 53 DHCP 报文类型枚举 |
| `DhcpHeader` | `op`, `htype`, `hlen`, `hops`, `xid`, `secs`, `flags`, `ciaddr`, `yiaddr`, `siaddr`, `giaddr`, `chaddr`; `isBroadcast()`, `macAddr()` | BOOTP/DHCP 固定报头视图 |
| `DhcpOption` / `DhcpOptIter` | `code`, `value`; `init`, `next`, `hasError`, `hasEndOption` | 自动跳过 PAD 并在 END 截停的 Option 迭代器 |
| `DhcpPacket` | `header`, `sname`, `boot_file`, `options_bytes`; `options()`, `findOption(code)`, `msgType()`, `leaseTime()`, `subnetMask()`, `serverId()`, `requestedIp()`, `hostname()` | 完整 DHCPv4 报文零拷贝视图 |
| `DhcpWriter` | `init`, `setCiaddr/Yiaddr/Siaddr/Giaddr`, `writePad`, `writeOption`, `writeMsgType`, `writeU32Option`, `writeIpOption`, `finish` | 零分配 DHCPv4 编码器 |

---

## 三、使用示例

```cp
import {
    DHCP_OP_REQUEST,
    OPT_HOSTNAME,
    DhcpMsgType,
    DhcpWriter,
    parseDhcp,
} from "cap:net";

fn buildDiscover() {
    let buf: [512]u8 = [0; 512];
    let w = DhcpWriter.init(buf[0..512], DHCP_OP_REQUEST, 0x12345678, [0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33], true);
    * = w.writeMsgType(DhcpMsgType.discover);
    * = w.writeOption(OPT_HOSTNAME, "cp-node-01");
    if (pkt := w.finish()) {
        if (d := parseDhcp(pkt)) {
            * = d.msgType();
            * = d.hostname();
        }
    }
}
```
