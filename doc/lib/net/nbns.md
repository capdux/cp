# `cap:nbns` — RFC 1001 / RFC 1002 NetBIOS Name Service 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配的 NetBIOS Name Service（UDP/137）线协议编解码库，支持 RFC 1001 §14.1 32 字节一级半字节编解码、NBNS Query 零分配编码与解析、NB (`0x0020`) 多 IP 地址表迭代以及 NBSTAT (`0x0021`) 节点名表与 MAC 地址解析。

## 导入

```cp
import {
    NBNS_UDP_PORT,
    NBNS_HEADER_SIZE,
    NBNS_ENCODED_NAME_LEN,
    NBNS_RAW_NAME_LEN,
    NBNS_TYPE_NB,
    NBNS_TYPE_NBSTAT,
    NBNS_CLASS_IN,
    OP_QUERY,
    OP_REGISTRATION,
    OP_RELEASE,
    OP_WACK,
    OP_REFRESH,
    SUFFIX_WORKSTATION,
    SUFFIX_MESSENGER,
    SUFFIX_FILE_SERVER,
    SUFFIX_DOMAIN_MASTER,
    SUFFIX_DOMAIN_CTRL,
    SUFFIX_MASTER_BROWSER,
    SUFFIX_BROWSER_ELECTION,
    NbnsName,
    encodeNbnsName,
    decodeNbnsName,
    NbnsHeader,
    NbnsAddrEntry,
    NbnsAddrIter,
    NbnsStatEntry,
    NbnsStatIter,
    NbnsPacket,
    isNbns,
    parseNbns,
    encodeNbnsQuery,
} from "cap:net";
```

## 核心 API

- **一级半字节编解码**：`encodeNbnsName(name, suffix, out): usize?`（自动转大写、右补空格至 15B + 1B suffix -> 32B `'A'..'P'`）、`decodeNbnsName(encoded): NbnsName?`（附 `trimmedLen()` 与 `nameSlice()`）。
- **报头与报文解析**：`parseNbns(data): NbnsPacket?`、`isNbns(data): bool`（`NbnsHeader` 提供 `isResponse()`, `opcode()`, `isAuthoritative()`, `isTruncated()`, `isBroadcast()`, `rcode()`）。
- **NB (`0x0020`) 地址迭代**：`pkt.addrs()` / `NbnsAddrIter.init(rdata)` 返回 `NbnsAddrEntry { is_group, ont, ip }`。
- **NBSTAT (`0x0021`) 节点状态迭代**：`pkt.stats()` / `NbnsStatIter.init(rdata)` 迭代 `NbnsStatEntry { name, flags }`（`isGroup()`, `isActive()`），并通过 `unitId()` 提取 6B MAC 地址。
- **零分配 Query 编码**：`encodeNbnsQuery(tr_id, name, suffix, broadcast, out): usize?`（输出 50 字节标准查询包）。

## 示例

```cp
import { encodeNbnsName, decodeNbnsName, SUFFIX_FILE_SERVER } from "cap:net";

fn example() {
    let encoded: [32]u8 = [0; 32];
    if (n := encodeNbnsName("fred", SUFFIX_FILE_SERVER, encoded[0..])) {
        if (decoded := decodeNbnsName(encoded[0..n])) {
            * = decoded.nameSlice(); // "FRED"
        }
    }
}
```
