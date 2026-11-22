# `cap:ldif` — RFC 2849 LDAP Data Interchange Format (`.ldif`) 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

`cap:ldif` 提供纯 CP、零堆分配的 RFC 2849 LDIF 文件解析能力，完整支持 `version: 1` 文档头、单空格折行续行合并与展开、`:` / `::`（Base64）/ `:<`（URL）属性类型识别、`dn:` 与 `changetype:`（`add`/`delete`/`modify`/`modrdn`/`moddn`）变更记录流以及单遍统计汇总。

## 导入方式

```cp
import {
    LdifValueKind,
    LdifChangeType,
    LdifAttr,
    LdifEntry,
    LdifStats,
    LdifAttrIter,
    LdifIter,
    isLdif,
    summarizeLdif,
} from "cap:net";
```

## 枚举与数据结构

### `LdifValueKind: u8`
- `plain`（`: ` 普通文本）
- `base64`（`:: ` Base64 编码值）
- `url`（`:< ` 外部 URL 引用）

### `LdifChangeType: u8`
- `content`（普通目录条目）
- `add`, `delete`, `modify`, `modrdn`, `moddn`

### `LdifAttr`
- `name: []const u8` — 属性名（零拷贝切片）
- `raw_value: []const u8` — 原始属性值切片（保留折行续行序列）
- `kind: LdifValueKind`
- 方法 `unfoldValue(out: []u8): usize?` — 按 RFC 2849 剥离 `\r\n ` 或 `\n ` 单空格折行并写入 `out`，返回写入长度；缓冲区不足返回 `null`

### `LdifEntry`
- `dn: []const u8` — Distinguished Name 切片
- `dn_is_base64: bool` — 是否为 `dn::` Base64 编码
- `change_type: LdifChangeType`
- `raw_record: []const u8` — 原始记录切片
- 方法 `attrs(): LdifAttrIter` — 迭代 `dn:` 之后的全部属性
- 方法 `findAttr(name: []const u8): LdifAttr?` — 大小写不敏感查找 LDAP 属性

### `LdifStats`
- `version`, `entry_count`, `add_count`, `modify_count`, `delete_count`, `attr_count`, `base64_attr_count: u32`

## 核心迭代器与函数

- `LdifAttrIter.init(slice)` / `.next()` — 迭代记录内全部属性（合并单空格续行、跳过 `#` 注释与 `modify` 的 `"-"` 分隔行）
- `LdifIter.init(data)` / `.next()` — 按空行分隔迭代多条 LDIF 记录，自动解析顶部 `version: 1` 并写入 `iter.version`
- `isLdif(data: []const u8): bool` — 快速判断是否为合法 LDIF 内容
- `summarizeLdif(data: []const u8): LdifStats` — 单遍汇总统计

## 快速示例

```cp
import { LdifIter, summarizeLdif } from "cap:net";

fn scanDirectory(ldif_bytes: []const u8) {
    let it = LdifIter.init(ldif_bytes);
    let buf: [256]u8 = [0; 256];
    while (entry := it.next()) {
        if (desc := entry.findAttr("description")) {
            if (n := desc.unfoldValue(buf[0..])) {
                * = buf[0..n];
            }
        }
    }
    const stats = summarizeLdif(ldif_bytes);
    * = stats.entry_count;
}
```
