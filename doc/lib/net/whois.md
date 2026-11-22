# `cap:whois` — RFC 3912 WHOIS 域名与 IP 注册信息查询协议零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**: `lib/whois/`  
> **导入协议**: `import { WHOIS_TCP_PORT, WHOIS_IANA_HOST, WHOIS_MAX_QUERY_LEN, isValidWhoisDomain, encodeWhoisQuery, encodeWhoisDomainQuery, WhoisField, WhoisFieldIter, findWhoisField, parseWhoisRefer, WhoisSummary, parseWhoisSummary } from "cap:net";`  
> **动态挂载**: `import cap from "cap";` -> `cap.whois.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:whois` 是纯 CP 实现、**零堆分配（zero-alloc）**、零 C/POSIX 外部依赖的 RFC 3912 WHOIS 协议编码与键值响应解析库：

1. **安全查询编码 (`encodeWhoisQuery` / `encodeWhoisDomainQuery`)**：
   - 将查询词编码为 RFC 3912 标准线缆格式 `query + "\r\n"` 直接写入调用方缓冲区。
   - 严格拒绝空查询、超长查询（`> 255` 字节）及包含 `\r`/`\n`/`\0` 的注入载荷。
   - `encodeWhoisDomainQuery` 额外校验 ASCII 域名标签规则（无空标签、首尾不为 `-`、仅含字母/数字/`-`/`.`）。
2. **零拷贝键值流式迭代 (`WhoisFieldIter` / `findWhoisField`)**：
   - 兼容 `\r\n` 与 `\n` 行尾，自动跳过空行、`#`/`%`/`>>>` 注释行及无冒号叙事行。
   - 按首个 `:` 切分 `key` 与 `value` 并剥离首尾空格与制表符（保留值内部的冒号，例如 ISO 8601 时间戳或 IPv6 地址）。
   - `findWhoisField` 忽略 ASCII 大小写查找首个匹配字段。
3. **Referral 提取与多注册局摘要归一 (`parseWhoisRefer` / `parseWhoisSummary`)**：
   - `parseWhoisRefer` 自动提取 IANA (`refer:`) 或 Registrar (`Registrar WHOIS Server:` / `Whois Server:` / `whois:`) 指向的下游 WHOIS 服务器。
   - `parseWhoisSummary` 单遍扫描归一 ICANN / Verisign / RIPE / ARIN 等主流注册局字段别名，并统计 `nameserver_count` 与 `status_count`。

---

## 二、 核心 API 速查表 (`lib/whois/whois.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| 协议常量 | `WHOIS_TCP_PORT (43)`, `WHOIS_IANA_HOST ("whois.iana.org")`, `WHOIS_MAX_QUERY_LEN (255)` |
| `isValidWhoisDomain(domain)` | 校验 ASCII 域名合法性 |
| `encodeWhoisQuery(query, out)` | 编码 `query + "\r\n"` 写入 `out: []u8`，防 `\r`/`\n`/`\0` 注入，返回写入字节数或 `null` |
| `encodeWhoisDomainQuery(domain, out)` | 校验域名合法性并编码为 WHOIS 查询 |
| `WhoisField` | `{ key: []const u8, value: []const u8 }`；方法 `keyEqualsIgnoreCase(target)` |
| `WhoisFieldIter` | `init(body: []const u8)`、`next(*_): WhoisField?` |
| `findWhoisField(body, key)` | 忽略 ASCII 大小写查找首个匹配字段值，返回 `([]const u8)?` |
| `parseWhoisRefer(body)` | 提取 `refer:` / `Registrar WHOIS Server:` 等目标主机名 |
| `WhoisSummary` | `{ domain_name, registrar, whois_server, created, updated, expires: []const u8, nameserver_count, status_count: usize }`；含 `hasDomain()`, `hasRegistrar()`, `hasWhoisServer()` |
| `parseWhoisSummary(body)` | 单遍归一 ICANN / Verisign / RIPE / ARIN 字段别名 |

---

## 三、 使用示例

```typescript
import { encodeWhoisDomainQuery, parseWhoisRefer, parseWhoisSummary } from "cap:net";

fn demo() {
    let qbuf: [260]u8 = [0; 260];
    if (n := encodeWhoisDomainQuery("example.com", qbuf[0..])) {
        // qbuf[0..n] == "example.com\r\n"
        * = n;
    }

    const iana_resp = "% IANA WHOIS server\nrefer:        whois.verisign-grs.com\n";
    if (host := parseWhoisRefer(iana_resp)) {
        // host == "whois.verisign-grs.com"
        * = host;
    }

    const verisign_resp =
        "   Domain Name: EXAMPLE.COM\r\n" +
        "   Registrar WHOIS Server: whois.iana.org\r\n" +
        "   Creation Date: 1995-08-14T04:00:00Z\r\n" +
        "   Name Server: A.IANA-SERVERS.NET\r\n";
    const summary = parseWhoisSummary(verisign_resp);
    * = summary;
}
```
