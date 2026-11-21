# cap:dns_txt_parse

> 源码吸收自 `/data/dev/bun/src/dns.zig`（RFC 7489 DMARC、RFC 7208 SPF 与通用 DNS TXT 键值标签规范），提供纯 CP、零外部依赖、零堆分配的 DNS TXT 标签、DMARC 与 SPF 流式解析库。

## 导入方式

```cp
import {
    TxtTag,
    TxtTagIter,
    findTxtTag,
    DmarcPolicy,
    DmarcRecord,
    SpfQualifier,
    SpfTerm,
    SpfIter,
} from "cap:dns_txt_parse";
```

## 核心类型与 API

- `TxtTag`（`key: []const u8`, `value: []const u8`）与 `TxtTagIter`（`init` / `next(*_): TxtTag?`）：解析分号分隔的 `k=v; k2=v2` TXT 记录（如 DMARC / DKIM / BIMI）。
- `findTxtTag(record: []const u8, key: []const u8): ([]const u8)?`：大小写不敏感检索指定标签值。
- `DmarcPolicy`（`none` / `quarantine` / `reject`）与 `DmarcRecord`（`policy`, `subdomain_policy: DmarcPolicy?`, `pct: u8`, `rua: ([]const u8)?`, `ruf: ([]const u8)?`）：
  - `DmarcRecord.parse(record: []const u8): DmarcRecord?`：校验首标签 `v=DMARC1`、必选 `p=` 与 `pct`（默认 `100`，范围 `0..=100`）。
- `SpfQualifier`（`pass` `+`, `fail` `-`, `softfail` `~`, `neutral` `?`）、`SpfTerm`（`qualifier`, `mechanism`, `value: ([]const u8)?`）与 `SpfIter`（`init` / `isValid` / `next(*_): SpfTerm?`）：校验 `v=spf1` 前缀后按空白流式提取各机制与修饰词。

## 使用示例

```cp
import { DmarcRecord, SpfIter, findTxtTag } from "cap:dns_txt_parse";

fn example() {
    const dmarc_txt = "v=DMARC1; p=reject; sp=quarantine; pct=50; rua=mailto:dmarc@example.com";
    if (d := DmarcRecord.parse(dmarc_txt)) {
        * = d.pct; // 50
    }
    let spf = SpfIter.init("v=spf1 include:_spf.google.com ip4:192.0.2.0/24 ~all");
    while (term := spf.next()) {
        * = term;
    }
    * = findTxtTag("v=DKIM1; k=rsa; p=MIGfMA0GCSq", "K"); // "rsa"
}
```
