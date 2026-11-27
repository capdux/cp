[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/jcs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-json-stable-stringify` ([https://github.com/epoberezkin/fast-json-stable-stringify](https://github.com/epoberezkin/fast-json-stable-stringify))（原库名 `fast-json-stable-stringify` → CP 包名 `@cap/jcs`）

零堆分配的 RFC 8785 JSON Canonicalization Scheme (JCS) 确定性规范化 JSON 引擎 — 洁净室 MIT 实现，对标 npm `fast-json-stable-stringify` / `canonicalize` 与 Rust `serde_jcs`，适用于数字签名、Webhook HMAC、缓存 Key 与内容寻址。

Zero-allocation RFC 8785 JSON Canonicalization Scheme (JCS) deterministic serializer & semantic equivalence checker for CP — clean-room MIT parity with npm `fast-json-stable-stringify`, `canonicalize`, and Rust `serde_jcs`.

## 特性 / Features

- **RFC 8785 键序排序 / Deterministic UTF-16 Key Ordering**：递归将所有 JSON 对象的 Key 按 UTF-16 码元顺序确定性排序，同时保持数组元素顺序不变。
- **字符串与数字规范化 / String & Number Normalization**：去除所有冗余空白、支持 `\uXXXX` 与代理对规范化解码、规范化 `-0` / `-0.0` → `0` 及小数尾随零。
- **严格校验与安全护栏 / Strict Validation & Safety**：拒绝尾逗号、前导零数字、未闭合字符串/括号及尾随垃圾；固定最大嵌套深度 16、单对象最多 64 键，彻底零堆分配防栈溢出。

## 安装 / Install

```bash
cap add @cap/jcs
```

## API 参考 / API Reference

| 符号 / Symbol | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `JcsResult` | `{ ok: bool, len: usize, output: string }` | 规范化结果结构体 |
| `canonicalizeJson` | `(input: string, out: []u8) -> JcsResult` | 按 RFC 8785 规范化 JSON 到调用方 `out` 缓冲区 |
| `areJsonEquivalent` | `(a: string, b: string) -> bool` | 比较两段 JSON 语义是否等价（忽略空白与键顺序） |

## 使用示例 / Usage

```cp
import { canonicalizeJson, areJsonEquivalent } from "@cap/jcs";

let buf: [1024]u8 = undefined;
const res = canonicalizeJson("{\"z\": 1, \"a\": [true, -0, null], \"m\": {\"y\": 2, \"x\": 3}}", buf[0..1024]);
// res.ok == true
// res.output => "{\"a\":[true,0,null],\"m\":{\"x\":3,\"y\":2},\"z\":1}"

const eq = areJsonEquivalent("{\"b\":1, \"a\":2}", "{ \"a\" : 2 , \"b\" : 1 }"); // true
```

## License

MIT
