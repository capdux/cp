# `cap:etag` — RFC 9110 HTTP ETag 与条件请求库

> 对标 Rust `etag` crate + RFC 9110 §8.8.3 / §13.1 / §13.2.2 完整条件请求规范  
> 纯 CP 零堆分配实现，无 `any`、无外部运行时依赖，适合高频 HTTP 网关、静态文件服务与 REST API 乐观锁场景。

---

## 一、 设计架构

`cap:etag` 采用全栈零分配（Zero-Allocation）值类型设计：

1. **严格词法模型（`EntityTag`）**：直接切片借用输入缓冲中的 `opaque-tag`，严格校验 RFC 9110 §8.8.3 `etagc` 字符集（`0x21`、`0x23..0x7E`、`0x80..0xFF`），拒绝控制字符、空格与未转义双引号。
2. **多级生成器家族**：全部写入调用方提供的栈上缓冲（空间不足安全返回 `null`）：
   - **FNV-1a 64-bit**（`generateStrong` / `generateWeak`）：极速轻量 16 位十六进制摘要；
   - **128-bit 双哈希 + 长度编码**（`generateContentStrong` / `generateContentWeak`）：低碰撞率内容校验标签（`"<hex-len>-<32-hex>"`）；
   - **纯 CP SHA-256**（`generateSha256Strong` / `generateSha256Weak`）：符合 FIPS 180-4 的密码学级内容摘要；
   - **文件元数据弱标签**（`fromStat` / `fromStatStrong`）：按 `(size, mtime_ns)` 生成 `W/"<hex-size>-<hex-mtime_ns>"`，兼容负时间戳。
3. **流式头部列表解析器（`ETagListIter`）**：零分配扫描 `If-Match` / `If-None-Match` 逗号分隔列表，完整处理 `*` 通配符、OWS 空白与**双引号内部逗号**（如 `"a,b", W/"c,d"`）。
4. **RFC 9110 §13.2.2 条件请求决策机（`evaluatePreconditions`）**：严格按 RFC 优先级顺序（Step 1 `If-Match` → Step 3 `If-None-Match`）结合 HTTP 方法与资源存在状态，直接输出 `PreconditionResult`（`Proceed` / `NotModified304` / `PreconditionFailed412`）。

---

## 二、 全部导出 API 一览

### 1. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `EntityTag` | RFC 9110 §8.8.3 实体标签值类型（字段：`tag: []const u8`、`is_weak: bool`） |
| `ParsedETag` | `EntityTag` 的向后兼容别名 |
| `PreconditionResult` | 条件请求判定枚举：`Proceed` / `NotModified304` / `PreconditionFailed412` |
| `ETagListIter` | `If-Match` / `If-None-Match` 头部列表零分配迭代器 |

### 2. `EntityTag` 方法

| 方法签名 | 说明 |
| :--- | :--- |
| `EntityTag.strong(tag: []const u8): EntityTag?` | 构造校验后的强 ETag，含非法 `etagc` 返回 `null` |
| `EntityTag.weak(tag: []const u8): EntityTag?` | 构造校验后的弱 ETag，含非法 `etagc` 返回 `null` |
| `EntityTag.parse(input: []const u8): EntityTag?` | 宽松解析单个 ETag（自动裁剪外层与 `W/` 后 OWS） |
| `EntityTag.parseStrict(input: []const u8): EntityTag?` | 严格按 RFC 9110 §8.8.3 解析（禁止任意位置多余 OWS） |
| `isWeak(): bool` / `isStrong(): bool` | 判断是否为弱/强验证器 |
| `opaqueTag(): []const u8` | 获取去引号后的不透明标签切片 |
| `toWeak(): EntityTag` / `toStrong(): EntityTag` | 强/弱形态转换 |
| `strongEq(other: EntityTag): bool` | RFC 9110 §8.8.3.2 强比较（向后兼容别名 `strongMatch`） |
| `weakEq(other: EntityTag): bool` | RFC 9110 §8.8.3.2 弱比较（向后兼容别名 `weakMatch`） |
| `formattedLen(): usize` | 格式化为 HTTP 头所需的精确字节长度 |
| `format(out: []u8): ([]const u8)?` | 格式化为 `"tag"` 或 `W/"tag"` 写入 `out`（向后兼容别名 `toHeader`） |

### 3. 校验与头部列表工具

| 函数签名 | 说明 |
| :--- | :--- |
| `isValidEtagChar(c: u8): bool` | 校验单字节是否属于 RFC 9110 `etagc`（`0x21` / `0x23..0x7E` / `0x80..0xFF`） |
| `isValidOpaqueTag(tag: []const u8): bool` | 校验整段不透明标签是否全部合法 |
| `isWildcardHeader(header_val: []const u8): bool` | 判断裁剪 OWS 后是否为通配符 `*` |
| `ETagListIter.init(header_val)` | 初始化列表迭代器，提供 `isWildcard()`、`nextRaw()`、`next()` |
| `parseHeaderList(header_val, out: []EntityTag): usize` | 将头部中所有合法 ETag 解析到调用方切片并返回数量 |

### 4. 条件请求求值与决策机

| 函数签名 | 说明 |
| :--- | :--- |
| `ifMatch(etag, header_val): bool` | 使用**强比较**评估 `If-Match`（支持 `*` 与多值列表） |
| `ifNoneMatch(etag, header_val): bool` | 使用**弱比较**评估 `If-None-Match`（支持 `*` 与多值列表） |
| `ifMatchTag(current: EntityTag, header_val): bool` | 对已解析 `EntityTag` 执行 `If-Match` 强比较 |
| `ifNoneMatchTag(current: EntityTag, header_val): bool` | 对已解析 `EntityTag` 执行 `If-None-Match` 弱比较 |
| `ifRangeMatch(etag, header_val): bool` | RFC 9110 §13.1.5 `If-Range` 单标签强比较（不支持 `*` 与列表） |
| `evaluatePreconditions(method, current_etag, if_match, if_none_match): PreconditionResult` | RFC 9110 §13.2.2 完整条件请求决策机 |

### 5. ETag 生成器

| 函数签名 | 输出格式 | 最小缓冲需求 |
| :--- | :--- | :--- |
| `generateStrong(content, out)` | `"<16-hex-fnv1a64>"` | 18 字节 |
| `generateWeak(content, out)` | `W/"<16-hex-fnv1a64>"` | 20 字节 |
| `generateContentStrong(content, out)` | `"<hex-len>-<32-hex-128bit>"` | 最多 51 字节 |
| `generateContentWeak(content, out)` | `W/"<hex-len>-<32-hex-128bit>"` | 最多 53 字节 |
| `generateSha256Strong(content, out)` | `"<64-hex-sha256>"` | 66 字节 |
| `generateSha256Weak(content, out)` | `W/"<64-hex-sha256>"` | 68 字节 |
| `fromStat(size: u64, mtime_ns: i128, out)` | `W/"<hex-size>-<hex-mtime_ns>"` | 最多 54 字节 |
| `fromStatStrong(size: u64, mtime_ns: i128, out)` | `"<hex-size>-<hex-mtime_ns>"` | 最多 52 字节 |

---

## 三、 RFC 9110 强/弱比较语义与优先级规则

### 1. 强比较 vs 弱比较真值表（RFC 9110 §8.8.3.2）

| ETag 1 | ETag 2 | `strongEq`（强比较） | `weakEq`（弱比较） |
| :--- | :--- | :---: | :---: |
| `"1"` | `"1"` | `true` | `true` |
| `"1"` | `"2"` | `false` | `false` |
| `"1"` | `W/"1"` | `false` | `true` |
| `W/"1"` | `"1"` | `false` | `true` |
| `W/"1"` | `W/"1"` | `false` | `true` |

- **弱比较（`weakEq` / `If-None-Match`）**：仅比较不透明标签内容是否逐字节一致，适用于 `GET` / `HEAD` 缓存验证（即使响应经 gzip 压缩变为弱 ETag 仍可命中 `304 Not Modified`）。
- **强比较（`strongEq` / `If-Match` / `If-Range`）**：要求双方均为强 ETag 且内容逐字节一致，确保字节级完全等价，适用于 `PUT` / `PATCH` / `DELETE` 乐观锁更新与断点续传校验。

### 2. 条件请求判定优先级（RFC 9110 §13.2.2）

1. **Step 1 — `If-Match`（优先判定）**：
   - 若资源不存在（`current_etag == null`），无论头部是 `*` 还是列表，一律返回 `PreconditionFailed412`；
   - 若资源存在且头部非 `*`，执行**强比较**，未命中则返回 `PreconditionFailed412`。
2. **Step 3 — `If-None-Match`**：
   - 若头部为 `*`：资源存在即视为命中，资源不存在则放行（经典 `PUT` + `If-None-Match: *` 防覆盖创建）；
   - 若头部为列表：资源存在且**弱比较**命中任一标签即视为命中；
   - 命中时：`GET` / `HEAD` 返回 `NotModified304`，其余方法（`PUT` / `POST` / `DELETE` / `PATCH`）返回 `PreconditionFailed412`。
3. **默认放行**：所有前置条件均满足时返回 `Proceed`。

---

## 四、 完整使用示例

### 示例 1：多种 ETag 生成、严格解析与格式化

```cp
import {
    EntityTag,
    generateContentStrong,
    generateSha256Strong,
    fromStat,
} from "cap:etag";
import { print } from "cap:io";

pub fn demoGenerateAndFormat() {
    const body = "{\"version\":42,\"status\":\"ok\"}";

    // 1. 高速 128-bit 哈希 + 长度编码强 ETag
    let fast_buf: [64]u8 = [0; 64];
    if (fast_etag := generateContentStrong(body, fast_buf[0..])) {
        print("Fast ETag: {s}\n", fast_etag);
    }

    // 2. 密码学级 SHA-256 强 ETag
    let sha_buf: [80]u8 = [0; 80];
    if (sha_etag := generateSha256Strong(body, sha_buf[0..])) {
        print("SHA-256 ETag: {s}\n", sha_etag);
    }

    // 3. 基于文件元数据的弱 ETag (size=65536, mtime_ns=1732500000000000000)
    let stat_buf: [64]u8 = [0; 64];
    if (stat_etag := fromStat(65536, 1732500000000000000, stat_buf[0..])) {
        // 严格解析并格式化回写
        if (parsed := EntityTag.parseStrict(stat_etag)) {
            let out_buf: [64]u8 = [0; 64];
            if (formatted := parsed.format(out_buf[0..])) {
                print("Stat Weak ETag: {s} (weak={})\n", formatted, parsed.isWeak());
            }
        }
    }
}
```

### 示例 2：条件请求 304 / 412 判定与并发乐观锁

```cp
import {
    PreconditionResult,
    evaluatePreconditions,
} from "cap:etag";

pub fn checkCacheAndOptimisticLock() {
    const current = "\"v3-9f86d081\"";

    // 场景 A：浏览器携带多个候选 ETag 发起 GET 缓存验证 -> 命中弱比较，返回 304
    const r1 = evaluatePreconditions("GET", current, null, "\"v1\", W/\"v3-9f86d081\"");
    // r1 == PreconditionResult.NotModified304

    // 场景 B：客户端携带过期 ETag 发起 PUT 更新 -> 强比较失败，阻止覆盖，返回 412
    const r2 = evaluatePreconditions("PUT", current, "\"v2-old\"", null);
    // r2 == PreconditionResult.PreconditionFailed412

    // 场景 C：客户端携带弱 ETag 发起 PUT 乐观锁 -> If-Match 强制要求强比较，返回 412
    const r3 = evaluatePreconditions("PUT", current, "W/\"v3-9f86d081\"", null);
    // r3 == PreconditionResult.PreconditionFailed412

    // 场景 D：创建新资源时携带 If-None-Match: * 防重复创建
    const r4 = evaluatePreconditions("PUT", null, null, "*");
    // r4 == PreconditionResult.Proceed (资源尚不存在，允许创建)
    const r5 = evaluatePreconditions("PUT", current, null, "*");
    // r5 == PreconditionResult.PreconditionFailed412 (资源已存在，拒绝覆盖)
}
```

### 示例 3：Web 路由处理器集成（静态资源 + API 缓存控制）

```cp
import {
    PreconditionResult,
    evaluatePreconditions,
    fromStat,
    generateContentStrong,
} from "cap:etag";

pub struct HttpReply {
    status: u16,
    etag: []const u8,
    body: []const u8,
}

/// 静态文件路由：基于文件元数据生成弱 ETag 并执行 RFC 9110 条件判定
pub fn serveStaticAsset(
    method: []const u8,
    file_size: u64,
    mtime_ns: i128,
    file_bytes: []const u8,
    if_match: ([]const u8)?,
    if_none_match: ([]const u8)?,
    etag_buf: []u8,
): HttpReply {
    const etag = fromStat(file_size, mtime_ns, etag_buf) ?? "";
    const decision = evaluatePreconditions(method, etag, if_match, if_none_match);

    if (decision == PreconditionResult.NotModified304) {
        return HttpReply{ status: 304, etag: etag, body: "" };
    }
    if (decision == PreconditionResult.PreconditionFailed412) {
        return HttpReply{ status: 412, etag: etag, body: "Precondition Failed" };
    }
    return HttpReply{ status: 200, etag: etag, body: file_bytes };
}

/// REST API 资源更新路由：基于内容强 ETag 执行乐观锁校验
pub fn updateDocument(
    existing_body: ([]const u8)?,
    new_body: []const u8,
    if_match: ([]const u8)?,
    if_none_match: ([]const u8)?,
    etag_buf: []u8,
): HttpReply {
    let cur_etag: ([]const u8)? = null;
    if (body := existing_body) {
        cur_etag = generateContentStrong(body, etag_buf);
    }

    const decision = evaluatePreconditions("PUT", cur_etag, if_match, if_none_match);
    if (decision == PreconditionResult.PreconditionFailed412) {
        return HttpReply{ status: 412, etag: cur_etag ?? "", body: "ETag mismatch" };
    }

    const next_etag = generateContentStrong(new_body, etag_buf) ?? "";
    return HttpReply{ status: 200, etag: next_etag, body: new_body };
}
```
