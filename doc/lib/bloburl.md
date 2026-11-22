# `cap:bloburl` — 零堆分配 W3C Blob Object URL 解析与注册表库

> 参考来源：/data/dev/bun/src/runtime/webcore/ObjectURLRegistry.cp | 吸收日期：2026-11-22

## 概述

`cap:bloburl` 吸收自 Bun 上游 WebCore Object URL 注册表实现（`src/runtime/webcore/ObjectURLRegistry.cp`），剥离了 JSC 运行时绑定与全局互斥锁堆表耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 `blob:` Object URL 解析、格式化、RFC 4122 UUID v4 生成以及固定容量注册/解析/吊销注册表（`BlobUrlRegistry`）。

## 上游行为契约与 CP 演进

- **Bun 上游设计要点（`ObjectURLRegistry.cp`）**：
  - Bun 使用全局线程安全单例 `ObjectURLRegistry`（`Mutex` + `AutoHashMap(UUID, *Entry)`）实现 `URL.createObjectURL(blob)`、`URL.revokeObjectURL(url)` 与 `resolveObjectURL(url)`。
  - 快速过滤常量 `specifier_len = "blob:".len + 36 = 41`：上游 `isBlobURL`、`Bun__revokeObjectURL` 与 `jsFunctionResolveObjectURL` 均先检查前缀 `"blob:"` 且长度 `>= 41`，不满足时直接返回 `false` / `undefined` 而不触发 UUID 解析与加锁，极大降低普通 URL 路径的判断开销。
  - **幂等静默吊销**：对未注册、已吊销或格式非法的 URL 调用 `revoke` 时不会报错，而是静默忽略（符合 W3C File API 规范）。
- **CP 零堆分配与双规范增强**：
  - **零堆分配**：注册表槽位池（`[]BlobUrlEntry`）与 URL 输出缓冲（`[]u8`）均由调用方栈上或静态池提供，无隐藏堆分配；内置无全局状态的 SplitMix64 确定性 UUID v4 生成器。
  - **双规范格式兼容**：同时支持 Bun 原生无 origin 的 `blob:<uuid>`（41 字节）与 W3C File API 带 origin 的 `blob:<origin>/<uuid>`（如 `blob:https://example.com/<uuid>`）。
  - **RFC 4122 UUID v4 精确编解码**：自动设置 version (`0x4x`) 与 variant (`10xx`) 位，支持大小写混合十六进制解析与规范小写 36 字符格式化。

---

## 导出清单

### 常量与结构体

- `BLOB_PREFIX = "blob:"`、`UUID_STR_LEN = 36`、`BLOB_URL_MIN_LEN = 41`
- `BlobUuid`：`{ bytes: [16]u8 }`，附 `eql`、`parse`、`format`
- `ParsedBlobUrl`：`{ origin, uuid }`
- `BlobUrlEntry`：`{ occupied, uuid, payload_id, content_type, size }`
- `BlobUrlRegistry`：`init`、`createObjectUrl`、`resolve`、`has`、`revoke`、`activeCount`

### 独立函数

| 函数签名 | 说明 |
| :--- | :--- |
| `isBlobUrl(url: []const u8): bool` | 快速前缀与最小长度校验（对应 Bun `isBlobURL`） |
| `parseBlobUrl(url: []const u8): ParsedBlobUrl?` | 解析 `blob:<uuid>` 或 `blob:<origin>/<uuid>` |
| `formatBlobUrl(origin, uuid, out_buf): usize?` | 格式化规范 `blob:` Object URL，缓冲不足返回 `null` |

---

## 边界与安全注意事项

1. **固定容量背压**：当 `BlobUrlRegistry` 槽位池已满或 `out_buf` 长度不足时，`createObjectUrl` 原子返回 `null`，不会产生半初始化条目或覆盖现有句柄。
2. **重复吊销与槽位复用**：多次调用 `revoke` 幂等安全，释放后的槽位立即归还池中供后续 `createObjectUrl` 复用，无内存泄漏。
3. **大小写与前缀严格性**：`isBlobUrl` 与 `parseBlobUrl` 要求精确小写 `"blob:"` 前缀与合法 8-4-4-4-12 UUID 结构，拒绝截断或含非法十六进制字符的输入。
4. **并发访问**：CP 版 `BlobUrlRegistry` 本身不内置全局锁，若需跨线程共享，应由调用方在外层加锁或按 Actor 独占管理。

---

## 使用示例

```cp
import std from "std";
import { BlobUrlEntry, BlobUrlRegistry, isBlobUrl, parseBlobUrl } from "cap:bloburl";

fn blobUrlDemo() {
    let slots: [16]BlobUrlEntry = std.mem.zeroes([16]BlobUrlEntry);
    let slots_s: []BlobUrlEntry = slots[0..];
    let reg = BlobUrlRegistry.init(&slots_s, 42) ?? return;

    let url_buf: [128]u8 = std.mem.zeroes([128]u8);
    let url_s: []u8 = url_buf[0..];
    const n = reg.createObjectUrl(1001, "image/png", 4096, "https://app.local", &url_s) ?? return;

    const url = url_buf[0..n];
    if (isBlobUrl(url)) {
        if (parsed := parseBlobUrl(url)) {
            * = parsed; // parsed.origin == "https://app.local"
        }
    }

    const entry = reg.resolve(url);
    * = entry;
    * = reg.revoke(url); // 幂等吊销并回收槽位
}
```
