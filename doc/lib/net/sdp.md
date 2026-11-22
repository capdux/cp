# CP 标准库：RFC 8866 / RFC 4566 WebRTC SDP 会话描述解析与构建 (`cap:sdp`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 一、 架构总览与设计准则

`cap:sdp` 是 CP 语言官方内置的纯 CP **RFC 8866 / RFC 4566 SDP (Session Description Protocol) 解析与构建引擎**，专为 WebRTC Offer/Answer 信令协商、RTP 媒体编解码协商与 ICE 候选收集场景设计。全模块 **零外部 C 依赖、零 `std.posix`、零 `@import("bun")`**，所有字符串切片均零拷贝引用原始输入缓冲区。既支持 `import { ... } from "cap:net"` 具名导入，也支持 `import cap from "cap"; cap.sdp.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sdp/sdp.cp` | `// Source: cp/lib/sdp/sdp.cp (...)` | `SdpError`、`SdpAttribute`、`RtpCodec`、`IceCandidate`、`MediaSection`、`SdpSession` |
| `lib/sdp/index.cp` | `// Source: cp/lib/sdp/index.cp (Public Unified Export for cap:sdp & cap.sdp)` | `cap:sdp` & `cap.sdp` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合 (`SdpError`)

- `InvalidVersion`：缺失 `v=` 或版本号非 `0`。
- `MissingOrigin`：缺失 `o=` 会话发起者行或内容为空。
- `MissingSessionName`：缺失 `s=` 会话名称行。
- `MalformedLine`：非空行长度不足或缺少 `=` 分隔符。
- `InvalidMediaLine`：`m=` 媒体行缺少 `<media>`、`<port>`、`<proto>` 或 `<fmt>` 字段。
- `InvalidPort`：`m=` 媒体行端口号非合法 `u16` 整数。
- `OutOfMemory`：内存分配失败。

### 2. 数据结构 (`SdpAttribute` / `RtpCodec` / `IceCandidate`)

- **`SdpAttribute`**：`{ key: []const u8, value: ([]const u8)? }`。标志属性（如 `a=sendrecv`）的 `value` 为 `null`；键值属性（如 `a=ice-ufrag:xxxx`）的 `value` 为冒号后的切片。
- **`RtpCodec`**：`{ payload_type: u8, name: []const u8, clock_rate: u32, encoding_params: ([]const u8)? }`。由 `MediaSection.rtpmaps()` 从 `a=rtpmap:` 提取（如 `111 opus/48000/2`、`96 VP8/90000`）。
- **`IceCandidate`**：`{ foundation: []const u8, component: u32, transport: []const u8, priority: u64, address: []const u8, port: u16, cand_type: []const u8 }`。由 `MediaSection.candidates()` 从 `a=candidate:` 解析（支持 `host`、`srflx`、`prflx`、`relay` 及尾部扩展参数）。

### 3. 媒体段 (`MediaSection`)

- 字段：`media_type: []const u8`、`port: u16`、`proto: []const u8`、`formats: [][]const u8`、`connection: ([]const u8)?`、`attributes: []SdpAttribute`。
- 核心方法：
  - `getAttr(key: []const u8): ([]const u8)?`：查找首个匹配 `key` 的属性值。
  - `hasFlag(key: []const u8): bool`：判断是否存在名为 `key` 的属性。
  - `rtpmaps(allocator): ([]RtpCodec) ! SdpError`：提取该媒体段下的全部 `a=rtpmap:` 编解码参数。
  - `candidates(allocator): ([]IceCandidate) ! SdpError`：提取该媒体段下的全部 `a=candidate:` ICE 候选。
  - `direction(): ([]const u8)?`：返回 `"sendrecv"` / `"sendonly"` / `"recvonly"` / `"inactive"` 之一，未声明则返回 `null`。
  - `deinit(*_, allocator)`：释放 `formats` 与 `attributes` 切片。

### 4. 会话描述 (`SdpSession`)

- 字段：`version: u32`、`origin: []const u8`、`session_name: []const u8`、`connection: ([]const u8)?`、`timing: []const u8`、`attributes: []SdpAttribute`、`media: []MediaSection`。
- 核心方法：
  - `SdpSession.parse(allocator, raw_sdp): SdpSession ! SdpError`：解析完整 SDP（兼容 `\r\n` 与 `\n` 换行）。
  - `format(allocator): ([]u8) ! SdpError`：格式化为标准 `\r\n` 换行的 RFC 8866 SDP 字符串。
  - `getAttr(key: []const u8): ([]const u8)?`：查询会话级 `a=` 属性值。
  - `findMedia(media_type: []const u8): (*const MediaSection)?`：按媒体类型（如 `"audio"`、`"video"`、`"application"`）查找首个媒体段。
  - `deinit(*_, allocator)`：释放会话属性与全部媒体段内存。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SdpSession } from "cap:net";
import cap from "cap";

pub fn main() {
    const alloc = std.heap.page_allocator;
    const raw =
        "v=0\r\n" ++
        "o=- 12345 2 IN IP4 127.0.0.1\r\n" ++
        "s=-\r\n" ++
        "t=0 0\r\n" ++
        "a=group:BUNDLE 0\r\n" ++
        "m=audio 9 UDP/TLS/RTP/SAVPF 111\r\n" ++
        "c=IN IP4 0.0.0.0\r\n" ++
        "a=sendrecv\r\n" ++
        "a=rtpmap:111 opus/48000/2\r\n";

    let sess = SdpSession.parse(alloc, raw) catch unreachable;
    defer sess.deinit(alloc);

    if (audio := sess.findMedia("audio")) {
        const codecs = audio.rtpmaps(alloc) catch unreachable;
        defer alloc.free(codecs);
    }

    const formatted = cap.sdp.SdpSession.format(sess, alloc) catch unreachable;
    defer alloc.free(formatted);
}
```
