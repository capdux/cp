# `cap:cef` — 零分配 ArcSight Common Event Format (CEF Rev 25) SIEM 日志解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

> **模块路径**：`lib/cef/`（入口 `lib/cef/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 Micro Focus / ArcSight CEF Rev 25 规范：自动识别可选 Syslog 前缀并定位 `"CEF:"`，按未转义 `'|'` 切分前 7 段头部（正确区分 `\|` 与 `\\|`），统一映射数字与文本严重级别，支持含空格值与 `\=`/`\\`/`\n`/`\r` 转义的 Extension 流式解析，并提供零分配头部编码与反转义工具。

---

## 一、导入方式

```cp
import {
    CEF_PREFIX,
    CefEvent,
    CefExtPair,
    CefExtIter,
    isCef,
    parseCefLine,
    findCefExt,
    unescapeCefHeader,
    unescapeCefExtValue,
    formatCefHeader,
} from "cap:obs";
```

---

## 二、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `isCef(line)` | `(line: []const u8): bool` | 校验是否为合法 CEF 行（支持带或不带 Syslog 前缀） |
| `CefEvent` | `syslog_prefix`, `version: u8`, `device_vendor`, `device_product`, `device_version`, `signature_id`, `name`, `severity_raw`, `extension`; `severityLevel(): u8`, `extensions()`, `findExt(key)` | 零拷贝 CEF 事件视图 |
| `parseCefLine(line)` | `(line: []const u8): CefEvent?` | 解析单行 CEF 日志（严格切分 7 个未转义 `\|`） |
| `CefExtPair` / `CefExtIter` | `key`, `raw_value`; `init(extension)`, `next(*_): CefExtPair?` | 零分配迭代 Extension，按 `<space><valid_key>=` 边界切分并尊重转义 |
| `findCefExt(extension, key)` | `([]const u8)?` | 查找指定 Extension 键的原始值 |
| `unescapeCefHeader(raw, out)` | `(raw: []const u8, out: []u8): usize?` | 反转义头部字段中的 `\|` 与 `\\` |
| `unescapeCefExtValue(raw, out)` | `(raw: []const u8, out: []u8): usize?` | 反转义 Extension 值中的 `\=`、`\\`、`\n`、`\r` |
| `formatCefHeader(...)` | `(..., out: []u8): usize?` | 零分配编码 CEF 头部并自动转义 `\|` 与 `\` |

---

## 三、使用示例

```cp
import { parseCefLine, unescapeCefExtValue } from "cap:obs";

fn inspectCef(line: []const u8) {
    if (ev := parseCefLine(line)) {
        * = ev.severityLevel();
        if (msg := ev.findExt("msg")) {
            let buf: [256]u8 = [0; 256];
            if (n := unescapeCefExtValue(msg, buf[0..256])) {
                * = buf[0..n];
            }
        }
    }
}
```
