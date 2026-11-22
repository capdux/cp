# `cap:leef` — 零分配 IBM QRadar LEEF 1.0/2.0 SIEM 日志解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

> **模块路径**：`lib/leef/`（入口 `lib/leef/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 IBM QRadar Log Event Extended Format (LEEF 1.0 & 2.0) 规范：自动识别可选 Syslog 前缀，支持 LEEF 1.0（5 段 `|` 头部，固定 `'\t'` 分隔）与 LEEF 2.0（6 段 `|` 头部，支持单字符与 `0xHH`/`xHH` 十六进制自定义分隔符及空字段回退 `'\t'`），零拷贝迭代 `key=value` 属性并支持值内嵌 `=`，提供零分配头部编码器。

---

## 一、导入方式

```cp
import {
    LEEF_PREFIX,
    LEEF_DEFAULT_DELIM,
    LeefEvent,
    LeefAttr,
    LeefAttrIter,
    isLeef,
    parseLeefLine,
    findLeefAttr,
    formatLeefHeader,
} from "cap:obs";
```

---

## 二、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `LEEF_PREFIX` / `LEEF_DEFAULT_DELIM` | `"LEEF:"` / `'\t'` | 协议前缀与默认属性分隔符 |
| `isLeef(line)` | `(line: []const u8): bool` | 支持可选 Syslog 前缀，校验 `"LEEF:1.0\|"` 或 `"LEEF:2.0\|"` |
| `LeefEvent` | `syslog_prefix`, `version_major`, `version_minor`, `vendor`, `product`, `product_version`, `event_id`, `delimiter`, `attributes_slice`; `attrs()`, `findAttr(key)` | 零拷贝 LEEF 1.0/2.0 事件视图 |
| `parseLeefLine(line)` | `(line: []const u8): LeefEvent?` | 解析单行 LEEF 1.0/2.0 日志 |
| `LeefAttr` / `LeefAttrIter` | `key`, `value`; `init(attributes_slice, delimiter)`, `next(*_): LeefAttr?` | 按 `delimiter` 零分配切分 `key=value`（仅按首个 `=` 拆分） |
| `findLeefAttr(event, key)` | `([]const u8)?` | 查找指定属性键（如 `"src"`、`"usrName"`、`"sev"`） |
| `formatLeefHeader(...)` | `(version_major, vendor, product, product_ver, event_id, delimiter, out): usize?` | 零分配格式化 LEEF 1.0/2.0 头部（LEEF 2.0 对 `\|` 与控制符自动输出 `0xHH`） |

---

## 三、使用示例

```cp
import { parseLeefLine, findLeefAttr } from "cap:obs";

fn inspectLeef(line: []const u8) {
    if (ev := parseLeefLine(line)) {
        if (src := findLeefAttr(ev, "src")) {
            * = src;
        }
        let it = ev.attrs();
        while (attr := it.next()) {
            * = attr.key;
            * = attr.value;
        }
    }
}
```
