# `cap:geneve` — RFC 8926 Geneve 通用网络虚拟化封装零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP 实现、零堆分配的 RFC 8926 Geneve（Generic Network Virtualization Encapsulation）协议解析与编码库，支持 8B 固定头、24-bit VNI、OAM/Critical 标志位与 4B 对齐 TLV 选项。

## 导入

```cp
import {
    GENEVE_MIN_HEADER_SIZE,
    GENEVE_DEFAULT_PORT,
    GENEVE_VERSION,
    GENEVE_MAX_VNI,
    GENEVE_FLAG_OAM,
    GENEVE_FLAG_CRITICAL,
    GENEVE_OPT_CRITICAL_BIT,
    PROTO_ETHERNET,
    PROTO_IPV4,
    PROTO_IPV6,
    GeneveHeader,
    GeneveOption,
    GeneveOptionIter,
    GenevePacket,
    GeneveWriter,
    isGeneve,
    parseGeneve,
} from "cap:net";
```

## 常量

| 常量 | 值 | 说明 |
| :--- | :--- | :--- |
| `GENEVE_MIN_HEADER_SIZE` | `8` | 固定报头字节数 |
| `GENEVE_DEFAULT_PORT` | `6081` | IANA 默认 UDP 端口 |
| `GENEVE_VERSION` | `0` | RFC 8926 协议版本 |
| `GENEVE_MAX_VNI` | `0x00FFFFFF` | 24-bit VNI 最大值 |
| `GENEVE_FLAG_OAM` | `0x80` | 报头 OAM 标志位 (bit 7) |
| `GENEVE_FLAG_CRITICAL` | `0x40` | 报头关键选项标志位 C-flag (bit 6) |
| `GENEVE_OPT_CRITICAL_BIT` | `0x80` | 选项 Type 关键位 (bit 7) |
| `PROTO_ETHERNET` | `0x6558` | Transparent Ethernet Bridging (TEB) |
| `PROTO_IPV4` | `0x0800` | IPv4 内层载荷 |
| `PROTO_IPV6` | `0x86DD` | IPv6 内层载荷 |

## 核心 API

### 解码
- `isGeneve(data: []const u8): bool`：快速嗅探最小报头、`version == 0` 与选项长度边界。
- `parseGeneve(data: []const u8): GenevePacket?`：零拷贝解析 `GeneveHeader`、`options_slice` 与 `payload`。
- `GeneveHeader`：`isOam()`、`hasCriticalOptions()`、`headerBytes()`。
- `GenevePacket`：`options(): GeneveOptionIter`、`headerBytes(): usize`。
- `GeneveOptionIter`：`next(*_): GeneveOption?`，严格校验边界与选项 3-bit 保留位为 0，异常置 `had_error = true`。
- `GeneveOption`：`isCritical(): bool`、`typeWithoutCritical(): u8`。

### 编码
- `GeneveWriter.init(buf: []u8, proto_type: u16, vni: u32, is_oam: bool): GeneveWriter?`
- `writeOption(*_, opt_class: u16, opt_type: u8, opt_data: []const u8): bool`：校验 4B 对齐、单选项 <= 124B 与总选项 <= 252B，关键选项自动置位 C-flag。
- `finish(*_, inner_payload: []const u8): ([]const u8)?`：回填 `opt_len_words` 与 C-flag 并写入载荷。

## 使用示例

```cp
import { GeneveWriter, parseGeneve, PROTO_ETHERNET, GENEVE_OPT_CRITICAL_BIT } from "cap:net";

fn roundtrip() {
    let buf: [256]u8 = [0; 256];
    let w = GeneveWriter.init(buf[0..256], PROTO_ETHERNET, 0x123456, false) ?? return;
    const opt_val: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    * = w.writeOption(0x0102, GENEVE_OPT_CRITICAL_BIT | 0x05, opt_val[0..4]);
    const frame = w.finish("inner-frame") ?? return;

    const pkt = parseGeneve(frame) ?? return;
    let it = pkt.options();
    while (opt := it.next()) {
        * = opt.isCritical();
    }
}
```
