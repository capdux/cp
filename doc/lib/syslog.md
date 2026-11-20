// Source: cp/doc/lib/syslog.md
# `cap:syslog` — RFC 5424 & RFC 3164 Syslog 解析与格式化引擎

纯 CP、零 C 依赖的工业级 Syslog（RFC 5424 与传统 BSD RFC 3164）报文解析器与序列化引擎，支持自动协议识别、结构化数据（`SD-ELEMENT` / `SD-PARAM`）转义还原、可选 UTF-8 BOM 处理及基于 `std.mem.Allocator` 的确定性内存管理。

## 导入方式

```cp
import {
    SyslogError,
    Facility,
    Severity,
    SdParam,
    SdElement,
    SyslogMessage,
    parse,
    parseRfc5424,
    parseRfc3164,
    formatRfc5424,
    formatRfc3164,
} from "cap:syslog";

// 或动态命名空间挂载：
import cap from "cap";
// cap.syslog.parse / cap.syslog.formatRfc5424 / cap.syslog.Facility
```

## 核心类型

### `SyslogError`
- `InvalidPriority`：`<PRI>` 缺失、包含非数字、位数超限或值大于 `191`
- `InvalidVersion`：RFC 5424 版本号缺失或不为 `1`
- `MalformedHeader`：头部字段缺失、空格分隔异常或包含非可打印 ASCII 字符
- `MalformedStructuredData`：结构化数据未闭合 `[`、参数语法错误、非法未转义 `]` 或尾部多余字符
- `InvalidTimestamp`：RFC 5424（ISO 8601 / RFC 3339）或 RFC 3164（`Mmm dd hh:mm:ss`）时间戳格式非法
- `OutOfMemory`：内存分配失败

### `Facility`（`0..23`）& `Severity`（`0..7`）
- `Facility`：包含 `Kern`（0）..`Local7`（23），提供 `Facility.fromCode(code: u8): Facility?`、`toCode(): u8` 与 `toString(): []const u8`。
- `Severity`：包含 `Emerg`（0）..`Debug`（7），提供 `Severity.fromCode(code: u8): Severity?`、`toCode(): u8` 与 `toString(): []const u8`。

### `SdParam` & `SdElement` & `SyslogMessage`
- `SdParam`：`{ name: []const u8, value: []const u8 }`，支持 `SdParam.init(name, value)`。
- `SdElement`：`{ id: []const u8, params: []SdParam }`，支持 `SdElement.init(id, params)` 与 `getParam(name: []const u8): ([]const u8)?`。
- `SyslogMessage`：
  - 字段：`facility`、`severity`、`priority`、`version`、`timestamp`、`hostname`、`app_name`、`proc_id`、`msg_id`、`structured_data`、`message`、`is_rfc5424`、`allocator`。
  - 方法：`getSdElement(id: []const u8): SdElement?`、`deinit(*_)`（释放解析过程中分配的所有堆内存）。

## 解析与格式化 API

- `parse(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError`
  - 自动识别 RFC 5424（`<PRI>VERSION ...`）与 RFC 3164 BSD Syslog（`<PRI>Mmm dd hh:mm:ss ...`）。
- `parseRfc5424(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError`
  - 严格校验 PRI（0..191）、版本号（1）、NILVALUE `-` 处理、多 `SD-ELEMENT` 与 `\"` / `\\` / `\]` 转义还原，以及可选 UTF-8 BOM（`\xEF\xBB\xBF`）剥离。
- `parseRfc3164(alloc: std.mem.Allocator, raw: []const u8): SyslogMessage ! SyslogError`
  - 解析传统 BSD 时间戳、`hostname` 与 `tag[pid]: msg`。
- `formatRfc5424(alloc: std.mem.Allocator, msg: SyslogMessage): ([]u8) ! SyslogError`
  - 自动计算 `PRI = facility * 8 + severity`、缺省字段输出 `-`、严格转义 SD-PARAM 值中的 `"`、`\`、`]`。
- `formatRfc3164(alloc: std.mem.Allocator, msg: SyslogMessage): ([]u8) ! SyslogError`
  - 生成标准 `<PRI>Mmm dd hh:mm:ss host tag[pid]: msg` 报文。
