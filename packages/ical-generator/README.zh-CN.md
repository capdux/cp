[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ical-generator

面向 CP 语言的零堆分配 RFC 5545 iCalendar（`.ics`）`VCALENDAR` / `VEVENT` 生成器、文本转义器与轻量事件解析器。

对齐 npm 源包 [`ical-generator`](https://github.com/sebbo2002/ical-generator)（MIT）。

## 安装与导入

```bash
cap add ical-generator
```

```cp
import {
    IcalEvent,
    ParsedEvent,
    IcalWriter,
    parseFirstEvent,
    unescapeText,
} from "ical-generator";
```

## 核心 API

### 数据结构

| 类名 | 字段 | 说明 |
| --- | --- | --- |
| `IcalEvent` | `uid`, `dtstart`, `dtend`, `summary`, `location`, `description`（均为 `&[]u8` / `[]const u8`） | 待写入的日程事件描述符；`location` / `description` 为空切片时自动省略。 |
| `ParsedEvent` | `uid_start/len`, `dtstart_start/len`, `dtend_start/len`, `summary_start/len`, `location_start/len: usize`, `valid: bool` | 指向原始 `.ics` 缓冲区的首个 `VEVENT` 零拷贝字节区间索引。 |

### `IcalWriter` 生成器类

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| `IcalWriter.init` | `(buf: []u8, prod_id: &[]u8): IcalWriter` | 在调用方缓冲区 `buf` 上初始化写入器，自动写入 `BEGIN:VCALENDAR`、`VERSION:2.0`、`PRODID` 与 `CALSCALE:GREGORIAN`。 |
| `addEvent` | `(*self, ev: IcalEvent): bool` | 追加一个 `VEVENT` 块并按 RFC 5545 自动转义文本属性；缓冲区溢出或已调用 `finish()` 时返回 `false`。 |
| `finish` | `(*self): i64` | 写入 `END:VCALENDAR\r\n` 并返回总字节数；失败或溢出返回 `-1`。 |
| `eventCount` | `(): usize` | 返回已成功写入的事件数量。 |
| `written` | `(): &[]u8` | 返回当前已写入的全部 `.ics` 字节只读切片。 |

### 解析与反转义函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `parseFirstEvent` | `(ics: &[]u8): ParsedEvent` | 解析 `ics` 中的首个 `VEVENT` 块并返回零拷贝区间（成功时 `valid == true`）。 |
| `unescapeText` | `(src: &[]u8, dst: []u8): i64` | 反转义 RFC 5545 文本（`\n`、`\,`、`\;`、`\\`）写入 `dst`，返回字节数或溢出时 `-1`。 |

## 使用示例

### 1. 生成 RFC 5545 `.ics` 日历

```cp
import { print } from "cap:io";
import { IcalWriter, IcalEvent } from "ical-generator";

pub fn main() {
    let buf: [2048]u8 = undefined;
    let w = IcalWriter.init(buf[0..2048], "-//CP//Calendar 1.0//EN");
    let ev = IcalEvent {
        uid: "evt-001@cp-lang.dev",
        dtstart: "20261127T090000Z",
        dtend: "20261127T100000Z",
        summary: "CP Release Sync, Team",
        location: "Room A; Floor 3",
        description: "Agenda:\n1. Compiler\n2. Stdlib",
    };
    let _ = w.addEvent(ev);
    let total = w.finish();
    if (total > 0) {
        print("{}\n", w.written());
    }
}
```

### 2. 解析并反转义 `VEVENT`

```cp
import { print } from "cap:io";
import { parseFirstEvent, unescapeText } from "ical-generator";

pub fn inspect(ics: []const u8) {
    let ev = parseFirstEvent(ics);
    if (ev.valid) {
        let raw_summary = ics[ev.summary_start .. ev.summary_start + ev.summary_len];
        let decoded: [256]u8 = undefined;
        let n = unescapeText(raw_summary, decoded[0..256]);
        if (n >= 0) {
            print("summary: {}\n", decoded[0..n]);
        }
    }
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配流式写出**：`IcalWriter` 直接向调用方 `[]u8` 缓冲区写入 CRLF 规范的 `.ics` 内容，并提供精确溢出状态（`failed` / `-1`）。
- **零拷贝区间解析**：`parseFirstEvent` 仅记录原始字节切片中的偏移与长度，不分配任何字符串对象。
- **原生 RFC 5545 转义**：自动处理 `\`、`,`、`;` 与 `\n` 的转义与反转义，无需正则或中间缓冲。
