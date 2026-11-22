# CP 标准库：LDAP 搜索过滤器与 DN 解析引擎 (`cap:ldap`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 一、 架构总览与设计准则

`cap:ldap` 是 CP 语言官方内置的纯 CP **RFC 4515 LDAP Search Filter** 解析、规范化、防注入转义、内存求值与 **RFC 4514 Distinguished Name (DN)** 解析引擎，**零外部依赖、零 `std.posix`、零 `@import("bun")`**。既可通过 `import { ... } from "cap:net"` 具名导入，也支持 `import cap from "cap"; cap.ldap.*` 编译期惰性挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ldap/ldap.cp` | `// Source: cp/lib/ldap/ldap.cp (...)` | `LdapError`、`LdapFilter` AST、`parseFilter`/`parseFilterWithMaxDepth`、`formatFilter`、`escapeFilterValue`、`matchesEntry`、`parseDn`/`freeDn` |
| `lib/ldap/index.cp` | `// Source: cp/lib/ldap/index.cp (Unified Exports for cap:ldap)` | `cap:ldap` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与数据结构

- **`LdapError`**：
  - `InvalidFilter`：空过滤器、空布尔子节点或非法操作符
  - `UnbalancedParens`：括号不匹配、缺少闭合 `)` 或存在未转义 `(` / `)`
  - `InvalidEscape`：非法 `\xx` 十六进制转义序列（如 `\zz` 或截断的 `\`）
  - `InvalidAttribute`：属性名或 OID 格式非法
  - `MaxDepthExceeded`：嵌套深度超过最大限制（默认 `DEFAULT_MAX_DEPTH = 64`）
  - `InvalidDn`：畸形 RFC 4514 DN（缺失 `=`、尾部多余 `,`、非法转义或空值）
  - `OutOfMemory`：内存分配失败
- **`LdapAttr`**：`{ name: []const u8, values: []const []const u8 }`，表示内存中的 LDAP 条目多值属性。
- **`DnComponent`**：`{ rdn_attr: []const u8, rdn_value: []const u8 }`，表示单个 RDN 组件。
- **`LdapFilter`**：带标签联合体（Tagged Union），包含：
  - `and_list: []LdapFilter`
  - `or_list: []LdapFilter`
  - `not_filter: *LdapFilter`
  - `present: []const u8`
  - `eq: LdapAttrVal` / `approx: LdapAttrVal` / `ge: LdapAttrVal` / `le: LdapAttrVal`（`{ attr, val }`）
  - `substring: LdapSubstring`（`{ attr, initial, any, final }`）
  - **`filter.deinit(allocator)`**：确定性递归释放 AST 全部内存。

### 2. 解析、规范化与防注入转义

- **`parseFilter(allocator, expr): LdapFilter!`** / **`parseFilterWithMaxDepth(allocator, expr, max_depth): LdapFilter!`**
  - 严格解析 RFC 4515 括号树、`(&...)`/`(|...)`/`(!...)` 布尔逻辑、`=`/`~=`/`>=`/`<=`/`=*`；
  - 完整解码 `\2a` (`*`)、`\28` (`(`)、`\29` (`)`)、`\5c` (`\`)、`\00` (`\0`) 等十六进制转义；
  - 将含未转义 `*` 的等值条件拆分为 `substring`（`initial` / `any` / `final`）；
  - 默认 64 层深度防护，超过返回 `LdapError.MaxDepthExceeded`。
- **`formatFilter(allocator, filter): ([]u8)!`**：将 `LdapFilter` 规范化序列化为标准 RFC 4515 字符串并安全转义特殊字符（调用方负责 `allocator.free`）。
- **`escapeFilterValue(allocator, raw): ([]u8)!`**：防止 LDAP 过滤器注入，将 `\`, `*`, `(`, `)`, `\0` 分别转义为 `\5c`, `\2a`, `\28`, `\29`, `\00`。

### 3. 内存条目求值与 DN 解析

- **`matchesEntry(filter, entry_attrs): bool`**：对 `[]const LdapAttr` 进行 ASCII 大小写不敏感匹配，支持多值属性、`present`、`eq`、`approx`、`ge`/`le`（整数优先比较，否则大小写不敏感字典序）、`substring` 首中尾通配符与布尔组合。
- **`parseDn(allocator, dn): ([]DnComponent)!`** / **`freeDn(allocator, components)`**：解析 RFC 4514 DN（支持 `\,`、`\"`、`\\`、`\20` 等转义与前导/尾随未转义空格裁剪），畸形 DN 返回 `LdapError.InvalidDn`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    LdapFilter, LdapAttr, DnComponent, LdapError,
    parseFilter, formatFilter, escapeFilterValue, matchesEntry, parseDn, freeDn,
} from "cap:net";
import cap from "cap";

pub fn demoLdap(): ! {
    const alloc = std.heap.page_allocator;

    let filter = parseFilter(alloc, "(&(|(cn=Alice)(mail=*@cp.org))(!(age>=10))(title=*))")?;
    defer filter.deinit(alloc);

    const formatted = formatFilter(alloc, filter)?;
    defer alloc.free(formatted);

    const cn_vals = [ "alice" ];
    const age_vals = [ "8" ];
    const title_vals = [ "Engineer" ];
    const attrs = [
        LdapAttr{ name: "CN", values: &cn_vals },
        LdapAttr{ name: "age", values: &age_vals },
        LdapAttr{ name: "Title", values: &title_vals },
    ];
    const matched = matchesEntry(filter, &attrs);
    _ = matched;

    const dn_parts = cap.ldap.parseDn(alloc, "cn=Alice\\, Bob,ou=Dev,dc=cp,dc=org")?;
    defer freeDn(alloc, dn_parts);
}
```
