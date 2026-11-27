[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# publicsuffix

> **源包溯源**：吸收自 **crates.io** [`publicsuffix`](https://github.com/rushmorem/publicsuffix)（`MIT OR Apache-2.0`），CP 官方洁净室实现 `publicsuffix@1.0.0`。

纯 CP 实现的零堆分配 Public Suffix List (PSL) 解析器，用于提取有效顶级域（eTLD）与可注册主域（eTLD+1），内置常见公共后缀并支持自定义通配符（`*.`）与例外（`!`）规则。

## 核心亮点

- **零堆分配**：`DomainInfo` 内联支持最长 256 字节规范化域名，`PslRuleList` 内联支持最多 64 条自定义规则（`PSL_MAX_RULES = 64`），全程零堆分配。
- **零栈清零优化**：域名规范化缓冲（`[256]u8`）与标签起始索引表（`[128]usize`）均采用 `undefined` 初始化，消除热路径栈清零开销。
- **完整 PSL 优先级规则**：支持普通规则（`co.uk`）、通配符规则（`*.ck`）与例外规则（`!www.ck`，例外优先于通配符）。
- **安全边界隔离**：精准区分公共后缀与可注册主域，适用于 Cookie 作用域校验、同站判定与 TLS 证书域控。

## 零成本迁移（Rust → CP）

直接沿用原包名 `publicsuffix`，无需改名：

**Rust：**
```rust
use publicsuffix::{List, Psl};
let domain = list.domain(b"www.example.co.uk").unwrap();
```

**CP：**
```typescript
import {
    parseDomain, suffix, registrableDomain, isPublicSuffix,
    PslRuleList, DomainInfo, PSL_MAX_RULES
} from "publicsuffix";
```

## 实战示例

### 1. 使用内置规则提取 eTLD 与可注册主域

```typescript
import { parseDomain, suffix, registrableDomain, isPublicSuffix } from "publicsuffix";

export fn demoPsl(): bool {
    let sfx = suffix("api.example.co.uk");          // "co.uk"
    let reg = registrableDomain("api.example.co.uk"); // "example.co.uk"
    let pub_only = isPublicSuffix("co.uk");         // true

    if (let info = parseDomain("WWW.Example.CO.UK")) {
        return info.suffixEq("co.uk") && info.registrableEq("example.co.uk") && pub_only && sfx != null && reg != null;
    }
    return false;
}
```

### 2. 添加自定义通配符与例外规则（`PslRuleList`）

```typescript
import { PslRuleList } from "publicsuffix";

export fn demoCustomRules(): bool {
    let mut list = PslRuleList.new(); // 包含内置规则
    _ = list.addRule("*.ck");
    _ = list.addRule("!www.ck");

    if (let info = list.parseDomain("www.ck")) {
        return info.suffixEq("ck") && info.registrableEq("www.ck");
    }
    return false;
}
```

## 核心 API 速览

- **顶层快捷函数**：`parseDomain(host): DomainInfo?`、`suffix(host): string?`、`registrableDomain(host): string?`、`isPublicSuffix(host): bool`。
- **`DomainInfo`**：`writeHost(out)`、`writeSuffix(out)`、`writeRegistrable(out)`、`suffixEq(s)`、`registrableEq(s)`、`hostEq(s)`，字段 `has_root`、`is_known_suffix`。
- **`PslRuleList`**：`new()`（含内置）、`empty()`（纯自定义）、`addRule(rule): bool`、`parseDomain`、`suffix`、`registrableDomain`、`isPublicSuffix`。
- **常量**：`PSL_MAX_RULES (64)`、`PSL_RULE_NORMAL (0)`、`PSL_RULE_WILDCARD (1)`、`PSL_RULE_EXCEPTION (2)`。

## 许可证

MIT
