# `cap:haproxy` — 纯 CP 零分配 HAProxy `.cfg` 负载均衡配置解析库

## 概述

`cap:haproxy` 提供纯 CP、零外部依赖、零堆分配的 HAProxy `.cfg` 配置段切分迭代器（`HaSectionIter`）、段内指令迭代器（`HaDirectiveIter`）以及段/指令零拷贝查找工具，与 `cap:nginx`、`cap:caddy` 组成完整的反向代理与负载均衡配置解析能力。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `HaSectionKind` | 配置段类型枚举：`global`, `defaults`, `frontend`, `backend`, `listen`, `resolvers`, `peers`, `userlist`, `other` |
| `HaSection` | 配置段结构体：`kind`, `name: ([]const u8)?`, `body: []const u8`（零拷贝切片指向原始输入） |
| `HaDirective` | 段内单行指令：`keyword: []const u8`, `args: []const u8`（已剥离引号外 `#` 行尾注释并去首尾空白） |
| `HaSectionIter` | 配置段流式迭代器：`init(input)`, `next(*_): HaSection?`（跳过空行与 `#` 注释行，按段头切分全部 HAProxy 段） |
| `HaDirectiveIter` | 段内指令流式迭代器：`init(section_body)`, `next(*_): HaDirective?`（逐行迭代，剥离单/双引号外 `#` 注释并保留 `\#` 转义） |
| `findHaSection(input, kind, name)` | 按段类型与可选段名查找首个匹配配置段（`name == null` 时匹配首个同类型段） |
| `findHaDirective(section_body, keyword)` | 在段体内按指令关键字查找首个匹配参数切片 |

## 示例

```cp
import {
    HaSectionKind,
    findHaSection,
    findHaDirective,
} from "cap:haproxy";

fn getApiBalance(cfg: []const u8): ([]const u8)? {
    if (sec := findHaSection(cfg, HaSectionKind.backend, "api_servers")) {
        return findHaDirective(sec.body, "balance");
    }
    return null;
}
```
