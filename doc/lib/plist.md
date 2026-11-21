# `cap:plist` — 纯 CP Apple XML Property List (`.plist`) 零分配解析与序列化库

## 概述

`cap:plist` 提供纯 CP、零外部依赖、零堆分配的 Apple XML Property List（macOS/iOS `Info.plist` / `LaunchAgent` 等）格式校验、顶层 `<dict>` 流式键值迭代（自动深度平衡跳过嵌套 `<dict>`/`<array>`）、强类型按键查询与 XML 转义序列化能力。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `PlistValueKind` | 值类型枚举：`string`, `integer`, `real`, `bool_true`, `bool_false`, `date`, `data`, `dict`, `array` |
| `PlistDictEntry` | 零拷贝键值对：`key`, `kind`, `raw_value` |
| `isXmlPlist(xml)` | 校验 `<plist>...</plist>` 结构（自动跳过 UTF-8 BOM、XML 声明与 `DOCTYPE`） |
| `PlistDictIter` | 顶层 `<dict>` 迭代器：`init(xml)`、`next(*_): PlistDictEntry?`，正确跳过嵌套 `<dict>`/`<array>` 子树 |
| `findPlistString(xml, key)` | 按键查找顶层 `<string>` 值 |
| `findPlistInt(xml, key)` | 按键查找顶层 `<integer>` 值并解析为 `i64` |
| `findPlistBool(xml, key)` | 按键查找顶层 `<true/>` / `<false/>` 布尔值 |
| `formatPlistStringEntry(key, value, out)` | XML 实体转义并输出 `<key>...</key>\n<string>...</string>\n`，缓冲不足返回 `null` |

## 示例

```cp
import { isXmlPlist, findPlistString, findPlistBool, formatPlistStringEntry } from "cap:plist";

fn example(xml: []const u8): bool {
    if (!isXmlPlist(xml)) return false;
    const bundle_id = findPlistString(xml, "CFBundleIdentifier") ?? return false;
    const high_dpi = findPlistBool(xml, "NSHighResolutionCapable") ?? false;
    let buf: [256]u8 = undefined;
    return bundle_id.len > 0 && high_dpi && formatPlistStringEntry("App", "CP & Co", buf[0..]) != null;
}
```
