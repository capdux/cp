# @cap/hosts-file

零堆分配的 POSIX / Windows `/etc/hosts` 文件解析、域名正反向查表与规范化生成引擎 — 洁净室 MIT 实现，对标 npm `hostile` 与 Rust `hostsfile`。

Zero-allocation POSIX / Windows `/etc/hosts` parser, case-insensitive forward/family/reverse lookup, and canonical formatter for CP — clean-room MIT parity with npm `hostile` and Rust `hostsfile`.

## 特性 / Features

- **完整 `/etc/hosts` 语法 / Full Hosts Syntax**：支持 LF/CRLF、跳过空行与整行/行尾 `#` 注释、自动展开单行多别名（如 `127.0.0.1 localhost api.local`）。
- **IPv4 与 IPv6 双栈 / Dual-Stack IPv4 & IPv6**：支持 `::1` 与带作用域地址 `fe80::1%lo0`，自动标注 `is_ipv6`。
- **正反向与按协议族查表 / Forward, Family & Reverse Lookup**：提供 `lookupIp`、`lookupIpByFamily`（区分 IPv4/IPv6）与 `reverseLookup`。
- **零堆分配 / Zero Heap Allocation**：所有 `HostEntry` 字符串直接零拷贝切片自输入缓冲区，`formatHosts` 写入调用方 `out: []u8`。

## 安装 / Install

```bash
cap add @cap/hosts-file
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `HostEntry` | `{ ip: string, hostname: string, is_ipv6: bool }` |
| `HostsTable` | `{ entries: [64]HostEntry, count: usize }` + `entryAt(i)` |
| `parseHosts(content)` | 解析 `/etc/hosts` 文本为 `HostsTable` |
| `lookupIp(table, hostname)` | 大小写不敏感查首条匹配 IP |
| `lookupIpByFamily(table, hostname, ipv6)` | 按协议族（IPv4/IPv6）查找匹配 IP |
| `reverseLookup(table, ip)` | 按 IP 反查主域名 |
| `formatHosts(table, out)` | 对齐并合并连续同 IP 别名，输出标准 `/etc/hosts` 格式 |

## 使用示例 / Usage

```cp
import { parseHosts, lookupIp, lookupIpByFamily, reverseLookup, formatHosts } from "@cap/hosts-file";

const table = parseHosts("127.0.0.1 localhost api.local\n::1 localhost\n");
const v4 = lookupIpByFamily(table, "LOCALHOST", false).?; // "127.0.0.1"
const v6 = lookupIpByFamily(table, "localhost", true).?;  // "::1"
const host = reverseLookup(table, "127.0.0.1").?;         // "localhost"

let buf: [512]u8 = undefined;
const rendered = formatHosts(table, buf[0..512]);
```

## License

MIT
