# CP 标准库：RFC 3261 SIP 报文解析、紧凑头规范化与构建 (`cap:sip`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 一、 架构总览与设计准则

`cap:sip` 是 CP 语言官方内置的纯 CP **RFC 3261 SIP (Session Initiation Protocol) 报文解析与构建引擎**，专为 VoIP / WebRTC 信令网关、SIP Proxy 与 IMS/SBC 协议栈设计。全模块 **零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，所有非折叠头与报文切片均零拷贝引用原始输入缓冲区。既支持 `import { ... } from "cap:net"` 具名导入，也支持 `import cap from "cap"; cap.sip.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sip/sip.cp` | `// Source: cp/lib/sip/sip.cp (...)` | `SipError`、`SipMethod`、`SipHeader`、`SipStartLine`、`SipMessage`、`parse`、`SipBuilder`、`normalizeHeaderName` |
| `lib/sip/index.cp` | `// Source: cp/lib/sip/index.cp (Public Unified Export for cap:sip & cap.sip)` | `cap:sip` & `cap.sip` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`SipError`)

- `InvalidVersion`：起始行中的 SIP 版本号非 `SIP/2.0`（如 `SIP/1.0`）。
- `InvalidStartLine`：请求行或状态行格式非法，或状态码不在 `100..699` 范围内。
- `MalformedHeader`：头行缺少 `:` 分隔符、头名为空或包含空白、或报文首个头行即为折叠延续行。
- `InvalidContentLength`：`Content-Length`（或紧凑头 `l`）的值非合法无符号整数。
- `TruncatedBody`：实际载荷字节数小于 `Content-Length` 声明的字节数。
- `HeaderInjection`：`SipBuilder` 检测到起始行或头字段名称/值包含 `\r`、`\n` 或 `\0`。
- `OutOfMemory`：内存分配失败。

### 2. 方法枚举与紧凑头映射

- **`SipMethod`**：`Invite`、`Ack`、`Bye`、`Cancel`、`Options`、`Register`、`Update`、`Info`、`Notify`、`Subscribe`、`Message`、`Other`，提供 `SipMethod.fromString(s)` 与 `toString()`。
- **`normalizeHeaderName(name: []const u8): []const u8`**：按 RFC 3261 §7.3.3 将单字符紧凑头名（大小写不敏感）规范化：
  - `v` → `Via`、`f` → `From`、`t` → `To`、`i` → `Call-ID`、`m` → `Contact`、`c` → `Content-Type`、`l` → `Content-Length`、`s` → `Subject`、`k` → `Supported`；其余名称原样返回。

### 3. 起始行与消息 (`SipStartLine` / `SipHeader` / `SipMessage`)

- **`SipHeader`**：`{ name: []const u8, value: []const u8 }`。
- **`SipStartLine`**：带标签联合体：
  - `request: { method: []const u8, uri: []const u8, version: []const u8 }`
  - `response: { version: []const u8, status_code: u16, reason: []const u8 }`
- **`SipMessage`**：`{ start_line: SipStartLine, headers: []SipHeader, body: []const u8 }`
  - `deinit(*_, allocator)`：释放解析过程中分配的头数组与折叠头缓冲。
  - `header(name: []const u8): ([]const u8)?`：大小写不敏感查找首个匹配头值，自动匹配紧凑别名。
  - `headersAll(allocator, name: []const u8): ([][]const u8) ! SipError`：收集全部匹配头值（如多条 `Via`），调用方需 `allocator.free(...)`。
  - `callId()` / `cseq()` / `viaTop()`：快捷提取 `Call-ID`、`CSeq` 与首条 `Via` 值。
  - `format(allocator): ([]u8) ! SipError`：序列化为标准 `\r\n` 换行的 SIP 报文。

### 4. 解析与构建 (`parse` / `SipBuilder`)

- **`parse(allocator, raw_bytes): SipMessage ! SipError`**：兼容 `\r\n` 与 `\n` 换行、支持 RFC 3261 §7.3.1 多行折叠头（Continuation Line），严格校验 `SIP/2.0`、状态码 `100..699` 及 `Content-Length` 截断。
- **`SipBuilder`**：
  - `SipBuilder.request(allocator, method, uri)` / `SipBuilder.response(allocator, status_code, reason)`
  - `header(*_, name, value): (*SipBuilder) ! SipError`（拦截 `\r`/`\n`/`\0` 注入）
  - `setBody(*_, body_bytes): *SipBuilder`
  - `build(*_): ([]u8) ! SipError`（自动补全 `Content-Length`）
  - `deinit(*_)`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { parse, SipBuilder } from "cap:net";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;
    let builder = SipBuilder.request(alloc, "REGISTER", "sip:registrar.example.com");
    defer builder.deinit();

    * = builder.header("Via", "SIP/2.0/UDP client.example.com:5060;branch=z9hG4bK776asdhds") catch unreachable;
    * = builder.header("From", "<sip:alice@example.com>;tag=1928301774") catch unreachable;
    * = builder.header("To", "<sip:alice@example.com>") catch unreachable;
    * = builder.header("Call-ID", "a84b4c76e66710@client.example.com") catch unreachable;
    * = builder.header("CSeq", "1 REGISTER") catch unreachable;

    const wire = builder.build() catch unreachable;
    defer alloc.free(wire);

    let msg = cap.sip.parse(alloc, wire) catch unreachable;
    defer msg.deinit(alloc);
}
```
