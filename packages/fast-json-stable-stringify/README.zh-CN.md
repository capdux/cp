[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# fast-json-stable-stringify

纯 CP 零堆分配 RFC 8785 JSON 规范化方案（JCS）确定性序列化与语义等价校验引擎 —— 洁净室 MIT 实现，对标 npm `fast-json-stable-stringify`、`canonicalize` 与 Rust `serde_jcs`。

递归按 RFC 8785 UTF-16 码元顺序排列所有 JSON 对象键、剔除多余空白、规范化 Unicode 转义与数字（如 `-0` / `-0.0` → `0`）并直接写入调用方栈缓冲，适用于数字签名、Webhook HMAC 验签、缓存 Key 与内容寻址。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `fast-json-stable-stringify`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/epoberezkin/fast-json-stable-stringify` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时保持原包名不变：

```ts
// TypeScript / Node.js (npm: fast-json-stable-stringify)
import stringify from "fast-json-stable-stringify";
const canonical = stringify({ z: 1, a: [true, -0, null] });
```

```cp
// CP (cap: fast-json-stable-stringify) — 零堆分配 RFC 8785 规范化输出
import { canonicalizeJson } from "fast-json-stable-stringify";

let buf: [1024]u8 = undefined;
const res = canonicalizeJson("{\"z\": 1, \"a\": [true, -0, null]}", buf[0..1024]);
// res.ok == true, res.output == "{\"a\":[true,0,null],\"z\":1}"
```

## 安装

```bash
cap add fast-json-stable-stringify
```

## 使用示例

### 基础用法：确定性签名规范化与语义等价判定

```cp
import { canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";

fn demoCanonical() {
    let buf: [1024]u8 = undefined;
    const raw = "{\"z\": 1, \"a\": [true, -0.0, null], \"m\": {\"y\": 2, \"x\": 3}}";
    const res = canonicalizeJson(raw, buf[0..1024]);
    if (res.ok) {
        // res.output == "{\"a\":[true,0,null],\"m\":{\"x\":3,\"y\":2},\"z\":1}"
        // res.len == res.output.len
    }

    // 忽略空白与对象键顺序比较两段 JSON 是否语义等价
    const same = areJsonEquivalent("{\"b\":1, \"a\":2}", "{ \"a\" : 2 , \"b\" : 1 }"); // true
    _ = res;
    _ = same;
}
```

### 边界用法：严格语法拒绝、缓冲区溢出与深度限制

```cp
import { canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";

fn demoEdgeCases() {
    let buf: [512]u8 = undefined;

    // 1. 严格拒绝尾逗号、前导零数字与尾随垃圾
    const bad1 = canonicalizeJson("{\"a\": 1,}", buf[0..512]);      // bad1.ok == false
    const bad2 = canonicalizeJson("{\"n\": 01}", buf[0..512]);      // bad2.ok == false
    const bad3 = canonicalizeJson("{\"a\": 1} extra", buf[0..512]); // bad3.ok == false

    // 2. 输出缓冲区不足时安全失败（ok == false）
    let tiny: [4]u8 = undefined;
    const overflow = canonicalizeJson("{\"key\":\"value\"}", tiny[0..4]); // overflow.ok == false

    // 3. 非法 JSON 永不视为等价
    const eq_bad = areJsonEquivalent("{bad}", "{bad}"); // false
    _ = bad1;
    _ = bad2;
    _ = bad3;
    _ = overflow;
    _ = eq_bad;
}
```

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `JcsResult` | `class { ok: bool, len: usize, output: string }` | 规范化结果；成功时 `output` 为 `out[0..len]` 切片 |
| `canonicalizeJson` | `fn(input: string, out: []u8): JcsResult` | 按 RFC 8785 规范化 JSON 至 `out`，畸形/溢出/超限时返回 `ok == false` |
| `areJsonEquivalent` | `fn(a: string, b: string): bool` | 判断两段 JSON 是否语义等价（单文档规范化上限 4096B） |
| `compareJsonKeys` | `fn(input: string, s1: usize, e1: usize, s2: usize, e2: usize): i8` | 底层 RFC 8785 UTF-16 码元键比较器（返回 `-1`/`0`/`1`） |

## 性能与安全亮点

- **16-lane SIMD 加速**：底层使用 `cap:ascii`（`isDigit`、`hexVal`、`hexCharLower`、`startsWith`）、`cap:bin`（`bytesEq`）与 `cap:fmt`（`writeStr`）加速词法解析与字节等价比较。
- **零堆分配与固定栈池**：单对象最多 64 键栈上排序（`[64]ObjEntry`），最大嵌套深度硬限 `MAX_DEPTH = 16`，防止恶意深层嵌套耗尽栈空间。
- **`undefined` 零清零优化**：内部键槽数组与等价比对缓冲（`[4096]u8 = undefined`）跳过冗余清零开销。
- **严格 RFC 合规校验**：确定性拒绝非法转义、尾逗号与缓冲溢出，绝不 Panic。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `fast-json-stable-stringify`。
