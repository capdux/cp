[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `url`

面向 CP 语言的纯 CP 零分配 WHATWG URL 标准解析器、规范化器、Origin 提取器与相对 URL 解析器——完整对标 **crates.io `url` v2**（`servo/rust-url`）。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `crates.io:url` ([servo/rust-url](https://github.com/servo/rust-url), v2 对齐) |
| **CP 包名与版本** | `url@1.0.0` (`MIT`) |
| **依赖** | `percent-encoding@1.0.0`、`idna@1.0.0` |

保持原包名导入，零改名成本直接迁移：

```rust
// Rust (crates.io url)
use url::Url;
let base = Url::parse("https://example.com:8443/api/v1/items?page=1#top")?;
let joined = base.join("../users")?;
```

```cp
// CP — 保持原包名，零堆分配栈结构体
import { WhatwgUrl } from "url";

const base = WhatwgUrl.parse("https://example.com:8443/api/v1/items?page=1#top");
const joined = WhatwgUrl.join(base, "../users");
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `WhatwgUrl.parse` | 96 B 完整 HTTPS URL | **487.78 ns/op** | **2,050,084 ops/s** | 187.69 MB/s | 16-lane SIMD 扫描、零堆分配 |

- **零堆分配**：`scheme`、`username`、`password`、`host`、`path`、`query`、`fragment` 全部内联于 `WhatwgUrl` 固定栈缓冲区中。
- **完整 WHATWG 规范化**：支持 IDNA 国际域名转码、IPv6 地址压缩、点段路径规约（`/a/b/../c` -> `/a/c`）、默认端口省略（`https:443` / `http:80`）与百分号编码。

## 实战示例

### 1. 解析、字段访问与规范化序列化

```cp
import { WhatwgUrl, defaultPortForScheme } from "url";

fn demoParse() {
    const u = WhatwgUrl.parse("https://alice:secret@Example.COM:443/a/b/../c?k=v#sec");
    if (!u.ok) { return; }

    const scheme = u.schemeStr();       // "https"
    const host = u.hostStr();           // "example.com"（自动小写化）
    const path = u.pathStr();           // "/a/c"（自动规约点段）
    const query = u.queryStr();         // "k=v"
    const frag = u.fragmentStr();       // "sec"
    const eff_port = u.effectivePort(); // 443（默认端口被省略，u.port == -1）
    const def_ws = defaultPortForScheme("wss"); // 443

    let out: [512]u8 = undefined;
    const n = u.toStr(out[0..]);        // "https://alice:secret@example.com/a/c?k=v#sec"
    _ = scheme; _ = host; _ = path; _ = query; _ = frag; _ = eff_port; _ = def_ws; _ = n;
}
```

### 2. 相对 URL 拼接（`join`）与 Origin 提取（`origin`）

```cp
import { WhatwgUrl } from "url";

fn demoJoinAndOrigin() {
    const base = WhatwgUrl.parse("https://api.example.com:8443/v1/users/list");
    if (!base.ok) { return; }

    const next = WhatwgUrl.join(base, "../profile?full=1");
    if (!next.ok) { return; }

    let url_buf: [512]u8 = undefined;
    const url_len = next.toStr(url_buf[0..]);
    // url_buf[0..url_len] == "https://api.example.com:8443/v1/profile?full=1"

    let orig_buf: [256]u8 = undefined;
    const orig_len = next.origin(orig_buf[0..]);
    // orig_buf[0..orig_len] == "https://api.example.com:8443"
    _ = url_len; _ = orig_len;
}
```

### 3. 进阶边界场景：IPv6 主机、非法 URL 校验与非特殊协议 Origin

```cp
import { WhatwgUrl } from "url";

fn demoEdgeCases() {
    // IPv6 字面量规范化压缩
    const v6 = WhatwgUrl.parse("http://[2001:0db8::1]:8080/status");
    if (v6.ok) {
        const host = v6.hostStr(); // "[2001:db8::1]"
        const port = v6.port;      // 8080
        _ = host; _ = port;
    }

    // 畸形 URL 安全返回 ok == false
    const bad = WhatwgUrl.parse("http://[::1:999999");
    if (!bad.ok) {
        // 拒绝非法输入
    }

    // 非特殊协议（如 mailto: / data:）的 origin 返回 "null"
    const mail = WhatwgUrl.parse("mailto:dev@example.com");
    let orig: [64]u8 = undefined;
    const n = mail.origin(orig[0..]); // "null"
    _ = n;
}
```

## 核心 API 速览

- `WhatwgUrl.parse(input)` — 解析并规范化绝对 URL
- `WhatwgUrl.join(base, rel)` — 基于 `base` 解析相对路径/引用
- 字段访问器：`schemeStr()`、`usernameStr()`、`passwordStr()`、`hostStr()`、`pathStr()`、`queryStr()`、`fragmentStr()`、`effectivePort()`
- 序列化：`toStr(out)`、`origin(out)`
- 工具函数：`defaultPortForScheme(scheme)`、`wuCloneUrl(src)`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
