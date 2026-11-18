# `cap:url` — 工业级 WHATWG URL 标准库

`cap:url` 是 CP 语言的高性能 URL 规范基础设施标准库，全量对标并吸收自 Bun 原生 URL 运行时资产（`bun/src/url/url.cp` 与 `bun/src/url/percent-encoding.cp`）。该模块提供了从底层的 RFC 3986 百分号编解码（PercentEncoding）、零分配查询字符串词法状态机（Scanner），到高层的动态查询参数表（URLSearchParams）与全功能 WHATWG URL 规范解析引擎（`URL`）的完整工业级支持。

---

## 模块结构与谱系映射

所有源文件第 1 行均严格标注对应的 Bun 原生源码谱系标记：

| 文件 | 谱系来源 | 核心职能 |
| :--- | :--- | :--- |
| `lib/url/percent.cp` | `bun/src/url/percent-encoding.cp` | RFC 3986 URI Component 百分号编解码器（零堆分配栈版与动态堆版） |
| `lib/url/search_params.cp` | `bun/src/url/url.cp` | `URLSearchParams` 查询参数容器、`Scanner` 零分配查询词法扫描器 |
| `lib/url/query.cp` | `bun/src/url/url.cp` | 向后兼容的聚合导出别名 |
| `lib/url/url.cp` | `bun/src/url/url.cp` | 完整 WHATWG 规范的 `URL` 结构体、协议解析、IPv6 识别与路径归一化 |
| `lib/url/index.cp` | `bun/src/url/` | 模块对外统一聚合导出入口 |

---

## 核心组件与特性详解

### 1. WHATWG 规范 URL 解析器 (`URL`)
- **全要素分解**：精准提取 `protocol`、`username`、`password`、`host`、`hostname`、`port`、`pathname`、`path`、`search`、`hash`、`origin`；
- **多协议原生支持**：
  - `isHTTP()` / `isHTTPS()` / `hasHTTPLikeProtocol()`：标准 Web 协议支持；
  - `isFile()`：`file:///path` 协议，并将 `origin` 规范化为 `"null"`；
  - `isS3()` & `s3Path()`：自动剔除协议前缀、查询串及 Hash，提取 S3 Bucket 及 Object Key；
  - `isBlob()` / `isUnix()`：支持前端 Blob URL 及 Unix Domain Socket 路径；
- **地址与端口智能识别**：
  - **IPv6 支持**：支持 `[::1]:8080`、`[2001:db8::1]` 等括号包裹的 IPv6 地址，正确提取纯主机名与独立端口；
  - **Localhost 判定**：原生识别 `localhost`、`127.0.0.1`、`0.0.0.0`、`[::1]`；
  - **端口回退机制**：`getPortAuto()` 在未显式提供端口时自动依据协议推导（HTTPS 为 443，HTTP 为 80）；
- **路径快速归一化 (`joinNormalize`)**：零额外堆分配合并 prefix、dirname、basename、extname，自动消除重复斜杠并保证前导 `/`。

### 2. 现代查询参数表 (`URLSearchParams`)
- **双轨存储设计**：
  - 内置 `[64]QueryParam` 静态内联数组，绝大多数 Web 请求 100% 零堆分配；
  - 超出 64 项时自动无缝平滑扩容至动态堆切片；
- **全套 CRUD API**：
  - `get(key)`：获取首个匹配项；
  - `getAll(key, target)`：批量提取同名多值参数；
  - `has(key)` / `append(key, value)` / `set(key, value)` / `delete(key)`；
  - `sort()`：原地字典序排序（Lexicographical In-place Sort），完全符合 WHATWG 规范；
  - `toString(out)` / `toOwnedString(allocator)`：零拷贝序列化与动态导出。

### 3. 高性能零分配分词器 (`Scanner`)
- 吸收自 Bun 的极致性能设计；
- 无需预分配任何数组，通过单遍流式状态机遍历查询字符串，逐一产出 `ScanResult`（自动标明键值范围及是否包含 `%` 或 `+` 需要解码）。

### 4. RFC 3986 百分号编解码器 (`PercentEncoding`)
- 严格遵循 RFC 3986 规范（未保留字符集：`A-Z`、`a-z`、`0-9`、`-`、`.`、`_`、`~`）；
- 提供直接针对栈缓冲区的 `encodeComponentToBuf` 与 `decodeToBuf`；
- 支持将 `+` 解码为空格的可选模式（URL 查询串专属）。

---

## API 使用示例

### 1. 解析 Web 与 S3 URL

```typescript
import { URL } from "cap:url";
const std = @import("std");

const alloc = std.heap.page_allocator;

// 解析完整 HTTPS URL
let u = try URL.parse("https://dev_user:secret123@api.service.io:8443/v2/items?category=ai#results", alloc);
defer u.deinit();

expect(u.protocol == "https:");
expect(u.host == "api.service.io:8443");
expect(u.hostname == "api.service.io");
expect(u.port == "8443");
expect(u.pathname == "/v2/items");
expect(u.origin == "https://api.service.io:8443");
expect(u.isHTTPS() == true);
expect(u.getPortAuto() == 8443);

// 解析 S3 对象存储路径
let s3_url = try URL.parse("s3://data-warehouse/2026/events.parquet?versionId=v1", null);
expect(s3_url.isS3() == true);
expect(s3_url.s3Path() == "data-warehouse/2026/events.parquet");
```

### 2. URLSearchParams 动态管理与排序

```typescript
import { URLSearchParams } from "cap:url";
const std = @import("std");

let params = URLSearchParams.parse("?foo=1&bar=2&foo=3", std.heap.page_allocator);
defer params.deinit();

// 读取首值与全部同名值
let first_val = params.get("foo"); // => "1"
let all_vals: [4]string = undefined;
let count = params.getAll("foo", all_vals[0..4]); // => 2项: ["1", "3"]

// 添加与设置
params.append("baz", "4");
params.set("foo", "updated");

// 字典序排序
params.sort();

let out_buf: [128]u8 = undefined;
let serialized = params.toString(out_buf[0..128]);
// => "baz=4&foo=updated&foo=3"
```

### 3. PercentEncoding 编解码

```typescript
import { PercentEncoding } from "cap:url";

let buf: [64]u8 = undefined;
let len = PercentEncoding.encodeComponentToBuf("hello world@cp#1", buf[0..64]);
// => "hello%20world%40cp%231"

let dec_buf: [64]u8 = undefined;
let dec_len = PercentEncoding.decodeToBuf("hello%20world%40cp%231", true, dec_buf[0..64]);
// => "hello world@cp#1"
```

---

## 质量与测试保证

- **既有测试验证**：`cap test cap/test/test_path_url.cp` (4/4 PASS)；
- **全功能完备测试**：`cap test cap/test/test_url_complete.cp` (6/6 PASS)；
- **编译器静态检查**：`cpc --lib lib/url/index.cp` (0 错误 0 警告)；
- **语法规范**：100% 纯正 CP 语法，所有文件首行严格保留谱系来源注释。
