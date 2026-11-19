# CP 标准库：WHATWG URL 解析、查询参数与百分号编码手册 (`cap:url`)

## 一、 架构总览与设计准则

`cap:url` 是 CP 语言官方内置的工业级 URL 解析与 URI 编解码标准库，全量对标并吸收自 Bun 原生 URL 运行时实现（`/data/dev/bun/src/url/url.cp`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     WHATWG URL 解析引擎 (URL)     │   查询参数容器 (URLSearchParams)   │
│  protocol / scheme / username     │  双轨存储: 64 项栈内联 + 动态堆扩容 │
│  password / host / hostname / port│  get / getAll / set / append       │
│  pathname / search / hash / origin│  delete / sort / getDecoded        │
├───────────────────────────────────┼────────────────────────────────────┤
│   零分配查询分词器 (Scanner)       │  RFC 3986 百分号编解码 (Percent)   │
│  流式单遍状态机产出 ScanResult     │  encodeComponentToBuf / decodeToBuf│
└───────────────────────────────────┴────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/url/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/url/url.cp` | `// Source: /data/dev/bun/src/url/url.cp` | WHATWG 规范 `URL`（别名 `Url`）结构体、组件拆解、特殊协议识别与路径归一化 |
| `lib/url/url_.cp` | `// Source: /data/dev/bun/src/url/url.cp` | 底层字符串切片搜索、IPv4/IPv6 地址判定、Localhost 识别与端口号安全解析原语 |
| `lib/url/search_params.cp` | `// Source: /data/dev/bun/src/url/url.cp` | `URLSearchParams`（别名 `SearchParams` / `Query`）双轨查询参数表与 `Scanner` 零分配分词器 |
| `lib/url/query.cp` | `// Source: /data/dev/bun/src/url/url.cp` | 查询参数子模块聚合重导出入口 |
| `lib/url/percent.cp` | `// Source: /data/dev/bun/src/url/url.cp` | `PercentEncoding`（别名 `Percent`）RFC 3986 URI 组件百分号编解码器 |
| `lib/url/index.cp` | `// Source: /data/dev/bun/src/url/url.cp` | `cap:url` 标准库对外统一导出入口 |

---

## 二、 WHATWG URL 解析器内存布局与组件拆解

### 1. 零拷贝切片视图内存布局

`URL` 解析器采用**原位切片引用（In-situ Slice Referencing）**设计：输入字符串 `href` 中的各个组件均直接以零拷贝切片（`string` / `[]const u8`）指向原输入缓冲区的对应偏移区间，无需为每个字段单独分配堆内存：

```text
https://admin:s3cr3t@api.example.com:8443/v2/models/chat?stream=true&temp=0.7#response
└──┬──┘ └─┬─┘ └──┬─┘ └──────┬──────┘ └┬─┘└──────┬──────┘└─────────┬─────────┘└───┬───┘
protocol  │   password   hostname    port    pathname          search           hash
("https:")│          └─────────┬────────┘
       username              host ("api.example.com:8443")
scheme: "https"      origin: "https://api.example.com:8443"
```

### 2. URL 核心字段与方法说明

- **组件字段**：
  - `href`: 原始完整 URL 字符串
  - `protocol`: 带冒号的协议名（如 `"https:"`、`"http:"`、`"file:"`、`"s3:"`）
  - `scheme`: 不带冒号的纯协议方案名（如 `"https"`、`"http"`）
  - `username` / `password`: 认证凭据（无凭据时为 `""`）
  - `host`: 主机名与显式端口（如 `"api.example.com:8443"` 或 `"[::1]:9090"`）
  - `hostname`: 纯主机名（IPv6 保留方括号，如 `"[::1]"`）
  - `port`: 显式端口字符串（未指定时为 `""`）
  - `pathname` / `path`: 路径部分（默认 `"/"`）
  - `search`: 带前导 `?` 的查询字符串（如 `"?stream=true&temp=0.7"`）
  - `hash`: 带前导 `#` 的片段标识符（如 `"#response"`）
  - `origin`: 规范化源（如 `"https://api.example.com:8443"`，`file://` 协议为 `"null"`）
  - `searchParams`: 自动解析的 `URLSearchParams` 实例
- **辅助判定与提取方法**：
  - `getPort(): u16?`：解析显式端口为 `u16`（无显式端口或非法时返回 `null`）
  - `getPortAuto(): u16`：返回显式端口，若未指定则自动回退至协议默认端口（HTTPS -> `443`，HTTP -> `80`）
  - `isHTTP()` / `isHTTPS()` / `hasHTTPLikeProtocol()` / `isFile()` / `isS3()` / `isBlob()` / `isUnix()`
  - `isIPAddress()` / `isLocalhost()`
  - `hostWithPath()`：返回不含尾随斜杠的 `host + pathname`
  - `s3Path()`：针对 `s3://bucket/key?query#hash` 提取纯 `"bucket/key"`
  - `URL.joinNormalize(out, prefix, dirname, basename, extname)`：栈上零分配归一化拼接 URL 路径

---

## 三、 `URLSearchParams` (`SearchParams` / `Query`) 查询参数操作

`URLSearchParams` 采用**双轨混合存储架构**：
1. **栈内联快路径**：内部预置 `entries: [64]QueryParam` 静态数组。对于参数数量 `<= 64` 的绝大多数 HTTP 请求，即使传入 `null` 分配器也能完成完整的 `parse`、`get`、`getAll`、`set`、`append`、`delete`、`sort` 与 `toString` 操作，实现 **100% 零堆分配**。
2. **动态扩容路径**：当参数超过 64 项且提供了 `Allocator` 时，自动平滑迁移至堆内存切片并按 2 倍几何级数扩容。

---

## 四、 `PercentEncoding` (`Percent`) RFC 3986 编码规范

严格遵循 **RFC 3986 Section 2.3** URI 组件百分号编码规范：
- **未保留字符集（Unreserved Characters）**：`ALPHA` (`A-Z`, `a-z`)、`DIGIT` (`0-9`)、`'-'`、`'.'`、`'@'` 之外的 `'_'`、`'~'` 原样保留；
- **保留与特殊字符**：全部编码为大写 `%XX` 十六进制序列（例如空格 `' '` -> `"%20"`，`'/'` -> `"%2F"`，`'@'` -> `"%40"`）；
- **表单兼容解码（`plus_as_space`）**：`decodeToBuf(input, true, out)` 或 `decodeQuery(input, out)` 支持将 `application/x-www-form-urlencoded` 中的 `'+'` 自动还原为空格 `' '`。

---

## 五、 完整代码示例

```typescript
import {
    URL,
    Url,
    URLSearchParams,
    SearchParams,
    Scanner,
    PercentEncoding,
    Percent,
} from "cap:url";
const std = @import("std");

pub fn demoUrlAndQuery(): !void {
    const alloc = std.heap.page_allocator;

    // =========================================================================
    // 1. 解析完整 WHATWG URL
    // =========================================================================
    let u = URL.parse(
        "https://agent:token99@cluster.internal:8443/v1/tasks/run?env=prod&tag=ai&tag=gpu#logs",
        alloc
    )?;
    defer u.deinit();

    // 组件提取
    // u.protocol   => "https:"
    // u.scheme     => "https"
    // u.username   => "agent"
    // u.password   => "token99"
    // u.host       => "cluster.internal:8443"
    // u.hostname   => "cluster.internal"
    // u.port       => "8443"
    // u.getPort()  => 8443
    // u.pathname   => "/v1/tasks/run"
    // u.search     => "?env=prod&tag=ai&tag=gpu"
    // u.hash       => "#logs"
    // u.origin     => "https://cluster.internal:8443"

    // =========================================================================
    // 2. 操作 URLSearchParams / SearchParams
    // =========================================================================
    // 读取单值与同名多值
    const env_val = u.searchParams.get("env"); // "prod"
    let tags: [4]string = undefined;
    const tag_count = u.searchParams.getAll("tag", tags[0..4]); // 2 ("ai", "gpu")

    // 独立解析与修改查询参数
    let sp = SearchParams.parse("?z=last&a=first&m=mid", null);
    sp.set("a", "updated");
    sp.append("b", "second");
    sp.delete("m");
    sp.sort(); // 按键名原地字典序排序

    let qs_buf: [128]u8 = undefined;
    const serialized = sp.toString(&qs_buf);
    // serialized => "a=updated&b=second&z=last"

    // =========================================================================
    // 3. RFC 3986 百分号编解码 (PercentEncoding / Percent)
    // =========================================================================
    let enc_buf: [128]u8 = undefined;
    const enc_len = Percent.encodeComponentToBuf("user@domain.com/path?q=1 2", &enc_buf);
    // enc_buf[0..enc_len] => "user%40domain.com%2Fpath%3Fq%3D1%202"

    let dec_buf: [128]u8 = undefined;
    const dec_len = Percent.decodeToBuf("hello+world%21", true, &dec_buf);
    // dec_buf[0..dec_len] => "hello world!"
}
```

---

## 六、 与 Web 标准 `URL` / Bun API 对照表

| 功能特性 | Web 标准 (`URL` / `URLSearchParams`) | Bun (`bun/src/url/url.cp`) | CP 标准库 (`cap:url`) |
| :--- | :--- | :--- | :--- |
| **解析 URL** | `new URL(input)` | `URL.parse(input)` | `URL.parse(input, alloc)` / `URL.parseSlice(input)` |
| **协议提取** | `url.protocol` (`"https:"`) | `url.protocol` / `url.displayProtocol()` | `url.protocol` (`"https:"`) / `url.scheme` (`"https"`) / `url.displayProtocol()` |
| **认证信息** | `url.username` / `url.password` | `url.username` / `url.password` | `url.username` / `url.password` |
| **主机与端口** | `url.host` / `url.hostname` / `url.port` | `url.host` / `url.hostname` / `url.port` | `url.host` / `url.hostname` / `url.port` |
| **数值端口推导** | `Number(url.port) \|\| defaultPort` | `url.getPort()` / `url.getPortAuto()` | `url.getPort()` (`u16?`) / `url.getPortAuto()` (`u16`) |
| **路径、查询与锚点** | `url.pathname` / `url.search` / `url.hash` | `url.pathname` / `url.search` / `url.hash` | `url.pathname` / `url.search` / `url.hash` |
| **S3 路径提取** | *需手动截取* | `url.s3Path()` | `url.s3Path()` |
| **查询参数解析** | `new URLSearchParams(str)` | `QueryStringMap.init(alloc, str)` | `URLSearchParams.parse(str, alloc)` / `SearchParams.parse(str, alloc)` |
| **零分配流式扫描** | *不支持* | `Scanner.init(str)` -> `scanner.next()` | `Scanner.init(str)` -> `scanner.next()` |
| **查询参数排序** | `params.sort()` | *不支持原地排序* | `params.sort()` |
| **URI 组件编码** | `encodeURIComponent(str)` | `PercentEncoding.encode` | `PercentEncoding.encodeComponentToBuf(str, out)` / `PercentEncoding.encodeComponent(str, alloc)` |
| **URI 组件解码** | `decodeURIComponent(str)` | `PercentEncoding.decode(Writer, writer, str)` | `PercentEncoding.decodeToBuf(str, plus_as_space, out)` / `PercentEncoding.decodeAlloc(str, plus_as_space, alloc)` |
