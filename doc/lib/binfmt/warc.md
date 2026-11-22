# `cap:warc` — 零分配 ISO 28500 Web ARChive (`.warc`) 1.0/1.1 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/warc/`（入口 `lib/warc/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 ISO 28500 WARC 1.0 & 1.1 网页归档与 Common Crawl 语料容器规范：校验 `WARC/1.0` / `WARC/1.1` 版本行，零拷贝解析记录头字段，严格按 `Content-Length` 切片二进制负载（安全防溢出与截断），并校验/跳过尾随 `\r\n\r\n`（兼容 `\n\n`）记录分隔符。

---

## 一、导入方式

```cp
import {
    WARC_VERSION_1_0,
    WARC_VERSION_1_1,
    WarcRecordType,
    warcRecordTypeFromSlice,
    WarcField,
    WarcFieldIter,
    findWarcHeader,
    WarcRecord,
    WarcIter,
    WarcStats,
    isWarc,
    summarizeWarc,
} from "cap:binfmt";
```

---

## 二、核心常量与枚举

| 符号 | 值 / 成员 | 说明 |
| :--- | :--- | :--- |
| `WARC_VERSION_1_0` / `WARC_VERSION_1_1` | `"WARC/1.0"` / `"WARC/1.1"` | ISO 28500 版本行前缀 |
| `WarcRecordType: u8` | `warcinfo`, `response`, `resource`, `request`, `metadata`, `revisit`, `conversion`, `continuation`, `unknown` | ISO 28500 `WARC-Type` 记录类型 |

---

## 三、核心数据结构与 API

| 类型 / 函数 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `isWarc(data)` | `(data: []const u8): bool` | 校验以 `WARC/1.0\r\n` 或 `WARC/1.1\r\n`（兼容 `\n`）开头 |
| `WarcField` / `WarcFieldIter` | `name`, `value`; `init(headers_slice)`, `next(*_): WarcField?` | 逐行零拷贝解析记录头 `Name: Value` 字段 |
| `findWarcHeader(headers_slice, name)` | `([]const u8)?` | 大小写不敏感查找任意记录头字段 |
| `WarcRecord` | `version`, `record_type`, `record_id`, `date`, `target_uri`, `content_type`, `payload_digest`, `content_length`, `headers_slice`, `payload`; `findHeader(name)` | 零拷贝 WARC 记录视图，`payload` 精确指向 `content_length` 字节 |
| `WarcIter` | `WarcIter.init(data)` / `next(*_): WarcRecord?` / `had_error: bool` | 零分配流式迭代拼接 WARC 记录，校验 `Content-Length` 与尾随分隔符 |
| `WarcStats` / `summarizeWarc(data)` | `record_count`, `response_count`, `request_count`, `warcinfo_count`, `conversion_count`, `total_payload_bytes` | 单遍统计多记录 `.warc` 语料摘要 |

---

## 四、使用示例

```cp
import { WarcIter, WarcRecordType, summarizeWarc } from "cap:binfmt";

fn inspectWarc(data: []const u8) {
    if (stats := summarizeWarc(data)) {
        * = stats.response_count;
        * = stats.total_payload_bytes;
    }

    let it = WarcIter.init(data);
    while (rec := it.next()) {
        if (rec.record_type == WarcRecordType.response) {
            * = rec.target_uri;
            * = rec.payload;
        }
    }
}
```
