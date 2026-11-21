# cap:netrc

> 源码吸收自 `/data/dev/bun/src/install/npm_client.zig`（`.netrc` / `_netrc` 认证凭据规范），提供纯 CP、零外部依赖、零堆分配的流式 `.netrc` 解析与主机凭据查找库。

## 导入方式

```cp
import { NetrcEntry, NetrcIter, lookupNetrc } from "cap:netrc";
```

## 核心类型与 API

### `NetrcEntry`

- `machine: ([]const u8)?`：主机名（`default` 条目为 `null`）
- `is_default: bool`：是否为 `default` 兜底条目
- `login: ([]const u8)?`
- `password: ([]const u8)?`
- `account: ([]const u8)?`

### `NetrcIter`

- `NetrcIter.init(src: []const u8): NetrcIter`
- `next(*_): NetrcEntry?`：流式读取下一条记录，支持单行/多行格式、`#` 行注释、含空格的双引号值、保留密码内 `#` 字符、以及跳过 `macdef <name>` 宏块直至空行。

### `lookupNetrc(src: []const u8, host: []const u8): NetrcEntry?`

按主机名大小写不敏感精确匹配 `machine` 条目；若未命中则自动回退到首个 `default` 条目。

## 使用示例

```cp
import { lookupNetrc } from "cap:netrc";

fn example() {
    const netrc = "machine api.github.com login bot password ghp_token\ndefault login guest password none\n";
    if (cred := lookupNetrc(netrc, "API.GITHUB.COM")) {
        * = cred.login;
        * = cred.password;
    }
}
```
