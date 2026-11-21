# `cap:caddy` — 纯 CP Caddyfile 零分配词法扫描与站点块解析库

## 概述

`cap:caddy` 提供纯 CP、零外部依赖、零堆分配的 Caddyfile 反向代理/TLS 站点配置词法扫描器（支持 `#` 行注释、双引号转义字符串、反引号原始字符串 `` `...` ``、`@matcher` 具名匹配器、行内 `{placeholder}` 占位符识别与换行界定）、顶层块迭代器（全局选项块、` (snippet) ` 与多地址站点块）及 64 层深度上限的结构平衡校验器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `CaddyTokenKind` | 词法单元类型：`word`, `string`, `matcher`, `lbrace`, `rbrace`, `newline`, `invalid` |
| `CaddyToken` | 零拷贝词法单元：`kind`, `lexeme` |
| `CaddyIter` | 流式词法扫描器：`init(input)`、`next(*_): CaddyToken?` |
| `CaddySiteBlock` | 顶层块元数据：`addresses`, `is_global`, `is_snippet`, `body` |
| `CaddySiteIter` | 顶层块迭代器：`init(input)`、`next(*_): CaddySiteBlock?` |
| `validateCaddyfile(input)` | 字符串/注释感知校验 `{}` 平衡、64 层深度上限与无未闭合引号 |

## 示例

```cp
import { CaddySiteIter, validateCaddyfile } from "cap:caddy";

fn inspectCaddyfile(cfg: []const u8): bool {
    if (!validateCaddyfile(cfg)) return false;
    let sites = CaddySiteIter.init(cfg);
    return sites.next() != null;
}
```
