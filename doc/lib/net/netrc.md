# `cap:netrc` — 零堆分配 `.netrc` / `_netrc` 认证凭据解析器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> 参考来源：/data/dev/bun/src/install/npm.cp | 吸收日期：2026-11-22

## 概述

`cap:netrc` 吸收自 Bun 包管理与 HTTP 客户端认证凭据体系（`src/install/npm.cp` 对应的 `.netrc` / `_netrc` 认证规范），提供纯 CP、零外部依赖、零堆分配的流式 `.netrc` 解析器与主机凭据查找库，适用于私有包仓库、Git/HTTP 客户端以及 CLI 自动认证。

## 行为契约与安全边界

- **多格式与标准 `.netrc` 语法兼容**：
  - 同时支持单行写法（`machine host login u password p`）与多行缩进写法；
  - 支持行首 `#` 注释，但**严格保留密码值内部的 `#` 字符**（防止含 `#` 的令牌或密码被误截断）；
  - 支持含空格的双引号值（如 `password "my secret pass"`）；
  - 完整跳过 `macdef <name>` 宏定义块直至遇到空行（双换行），防止宏体内容被误解析为主机条目。
- **主机匹配与 `default` 兜底**：`lookupNetrc(src, host)` 按主机名大小写不敏感精确匹配 `machine` 条目；若无精确匹配，自动回退到首个 `default` 条目（`is_default == true, machine == null`）。
- **零拷贝凭据视图**：所有解析字段直接切片借用自 `src`，不产生堆拷贝，便于调用方在使用完毕后立即清零原缓冲区内存。

## 导入方式

```cp
import { NetrcEntry, NetrcIter, lookupNetrc } from "cap:net";
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
- `next(*_): NetrcEntry?`：流式读取下一条记录，遍历结束返回 `null`。

### `lookupNetrc(src: []const u8, host: []const u8): NetrcEntry?`

按主机名大小写不敏感精确匹配 `machine` 条目；若未命中则自动回退到首个 `default` 条目。

## 安全注意事项

1. **文件权限检查由调用方负责**：按 Unix 惯例，读取 `~/.netrc` 前应确认文件权限为 `0600`（仅所有者可读写）。
2. **空白与不完整条目**：空文件或仅含注释/宏块的文件安全返回 `null`；缺少 `password` 或 `login` 的条目对应可选字段为 `null`。

## 使用示例

```cp
import { lookupNetrc } from "cap:net";

fn example() {
    const netrc = "machine api.github.com login bot password ghp_token#1\ndefault login guest password none\n";
    if (cred := lookupNetrc(netrc, "API.GITHUB.COM")) {
        * = cred.login;    // "bot"
        * = cred.password; // "ghp_token#1"
    }
}
```
