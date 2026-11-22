# `cap:scrypt` — RFC 7914 scrypt 内存硬化口令派生与 PHC 编解码

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 1. 概述

`cap:scrypt`（动态挂载 `cap.scrypt`）提供符合 RFC 7914 标准的 scrypt 内存硬化口令派生（KDF）与 PHC 字符串（`$scrypt$ln=...,r=...,p=...$salt$hash`）编解码及常量时间校验能力。底层复用 Zig 0.17 `std.crypto.pwhash.scrypt.kdf`，并在参数校验阶段强制限制 `ln ∈ [1, 22]`、`r >= 1`、`p >= 1` 及估算内存 `128 * r * (1 << ln) <= max_mem_bytes`，防止恶意超大参数引发 DoS。

## 2. 核心类型

- `ScryptParams`
  - `ln: u6`：$\log_2(N)$ 成本参数（合法范围 `1..=22`）。
  - `r: u30`：块大小参数（`>= 1`）。
  - `p: u30`：并行度参数（`>= 1`）。
  - `max_mem_bytes: usize`：允许的最大内存上限（字节），默认 `DEFAULT_MAX_MEM_BYTES`（64 MiB）。
  - 方法：`init(ln, r, p, max_mem_bytes)`、`fromN(n, r, p, max_mem_bytes)`、`interactive()`、`owasp()`、`estimatedMemBytes()`、`validate()`。
- `ParsedPhc`
  - `params: ScryptParams`、`salt: []u8`、`hash: []u8`。
  - 方法：`deinit(alloc)` 释放解码出的 `salt` 与 `hash`。

## 3. API 速查

- `hashRaw(alloc, password, salt, params, out_len) ![]u8`：执行原始 RFC 7914 scrypt 派生，返回 `out_len` 字节密钥（调用方负责 `alloc.free`）。
- `hashPhc(alloc, password, salt, params, out_len) ![]u8`：派生密钥并编码为标准 PHC 字符串（无填充 Base64）。
- `hashPhcRandom(alloc, password, params) ![]u8`：使用 `std.Io.Threaded` 生成 16 字节密码学随机盐并输出 32 字节哈希的 PHC 字符串。
- `parsePhc(alloc, phc) !ParsedPhc`：解析 `$scrypt$ln=...,r=...,p=...$salt$hash` 字符串。
- `verifyPhc(alloc, password, phc) !bool`：使用默认 64 MiB 内存上限对 PHC 字符串进行常量时间校验。
- `verifyPhcWithLimit(alloc, password, phc, max_mem_bytes) !bool`：带自定义内存上限的 PHC 常量时间校验。

## 4. 示例

```cp
import * as std from "std";
import * as scrypt from "cap:crypto";

fn example(alloc: std.mem.Allocator) !void {
    const params = scrypt.ScryptParams.interactive();
    const phc = try scrypt.hashPhcRandom(alloc, "correct-horse-battery-staple", params);
    defer alloc.free(phc);

    const ok = try scrypt.verifyPhc(alloc, "correct-horse-battery-staple", phc);
    // ok == true
}
```
