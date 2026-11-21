# cap:dns_caa_record

> 源码吸收自 `/data/dev/bun/src/dns.zig`（RFC 8659 DNS Certification Authority Authorization 规范），提供纯 CP、零外部依赖、零堆分配的 CAA 文本/线缆格式解析与 CA 签发策略判定库。

## 导入方式

```cp
import { CaaTag, CaaRecord, isCaAuthorized } from "cap:dns_caa_record";
```

## 核心类型与 API

- `CaaTag`：`issue`、`issuewild`、`iodef`、`unknown`。
- `CaaRecord`：
  - 字段：`flags: u8`、`tag_raw: []const u8`、`tag: CaaTag`、`value: []const u8`（已去外层双引号）。
  - `isCritical(): bool`：判断是否设置了关键位 `(flags & 0x80) != 0`。
  - `forbidsAllIssuance(): bool`：判断是否为禁止全部签发的空值或 `";"`。
  - `issuerDomain(): []const u8`：剥离 `;param=val` 参数后返回授权 CA 域名。
  - `CaaRecord.parseText(rdata: []const u8): CaaRecord?`：解析文本格式（如 `0 issue "letsencrypt.org"`、`128 issuewild ";"`）。
  - `CaaRecord.parseWire(rdata: []const u8): CaaRecord?`：解析 RFC 8659 §4.1 线缆格式 `[flags:1][tag_len:1][tag:tag_len][value:rest]`。
- `isCaAuthorized(records: []const CaaRecord, ca_domain: []const u8, is_wildcard: bool): bool`：按 RFC 8659 判定普通或通配符证书签发授权（未知 `critical` 标签直接拒绝；通配符优先匹配 `issuewild`，否则回退 `issue`；无相关约束默认允许）。

## 使用示例

```cp
import { CaaRecord, isCaAuthorized } from "cap:dns_caa_record";

fn example() {
    if (r1 := CaaRecord.parseText("0 issue \"letsencrypt.org; validationmethods=dns-01\"")) {
        if (r2 := CaaRecord.parseText("0 issuewild \";\"")) {
            const set: [2]CaaRecord = [r1, r2];
            * = isCaAuthorized(set[0..2], "letsencrypt.org", false); // true
            * = isCaAuthorized(set[0..2], "letsencrypt.org", true);  // false
        }
    }
}
```
