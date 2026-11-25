# `@cap/blake3`

工业级、纯 CP 实现、零外部依赖的 **BLAKE3** 密码学哈希库，严格对标官方 [BLAKE3 规范](https://github.com/BLAKE3-team/BLAKE3-specs) 与 Rust 官方 `blake3` crate 实现，并已通过全部官方标准测试向量交叉验证。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无需 C/FFI、无需 `unsafe`、无需外部静态/动态库，全平台零配置编译。
- **完整 Merkle 树 CV 栈**：完整实现 64 字节 Block、1024 字节 Chunk 与 54 层 CV 栈归并，支持任意长度输入（最高 $2^{64}$ 字节），正确处理跨多 Chunk（> 1024B / > 2048B / > 4096B）归并。
- **完整三大运行模式**：
  - 标准哈希（`hash` / `Blake3Hasher.init`）
  - 密钥哈希 / MAC（`keyedHash` / `Blake3Hasher.initKeyed`）
  - 密钥派生 / KDF（`deriveKey` / `Blake3Hasher.initDeriveKey`）
- **流式增量计算与 XOF 扩展输出**：支持任意分块 `update()`、非破坏性 `finalize()` / `finalizeXof(out)` 与复用 `reset()`。
- **侧信道安全与实用工具**：内置常量时间摘要比较 `hashEql` 与小写十六进制编码 `toHex`。

## 常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | :---: | :--- |
| `OUT_LEN` | `usize` | `32` | 默认摘要输出字节长度（256 位） |
| `KEY_LEN` | `usize` | `32` | 密钥哈希模式密钥长度（256 位） |
| `BLOCK_LEN` | `usize` | `64` | 压缩函数单块字节长度 |
| `CHUNK_LEN` | `usize` | `1024` | Merkle 树单叶节点（Chunk）字节长度 |

## API 文档

### 一次性便捷函数

- **`pub fn hash(data: []const u8): [32]u8`**  
  计算 `data` 的标准 32 字节 BLAKE3 摘要。

- **`pub fn keyedHash(key: [32]u8, data: []const u8): [32]u8`**  
  使用 32 字节密钥 `key` 计算 `data` 的密钥哈希（MAC）。

- **`pub fn deriveKey(context: string, key_material: []const u8): [32]u8`**  
  使用域分离上下文字符串 `context` 与输入密钥材料 `key_material` 派生 32 字节密钥。  
  > 建议 `context` 全局唯一、硬编码且包含应用名与用途描述，例如 `"my-app 2026-11-25 session encryption key"`。

### 流式哈希器 `Blake3Hasher`

- **`Blake3Hasher.init(): Blake3Hasher`**  
  创建标准模式流式哈希器。

- **`Blake3Hasher.initKeyed(key: [32]u8): Blake3Hasher`**  
  创建密钥哈希（MAC）模式流式哈希器。

- **`Blake3Hasher.initDeriveKey(context: string): Blake3Hasher`**  
  创建密钥派生（KDF）模式流式哈希器。

- **`hasher.update(input: []const u8)`**  
  增量喂入任意长度字节切片，可多次调用。

- **`hasher.finalize(): [32]u8`**  
  计算并返回 32 字节摘要。**非破坏性操作**：不修改内部状态，可继续 `update()` 或重复调用 `finalize()`。

- **`hasher.finalizeXof(out: []u8)`**  
  以 XOF（可扩展输出函数）模式将任意长度输出写入 `out`（例如 64B、128B、4096B）。前 32 字节与 `finalize()` 完全一致，且同样不破坏内部状态。

- **`hasher.reset()`**  
  将哈希器重置为空输入状态，保留原有的运行模式（标准 / 密钥 / KDF）与密钥/上下文，便于高频复用。

### 比较与编码工具

- **`pub fn hashEql(a: [32]u8, b: [32]u8): bool`**  
  常量时间比较两个 32 字节摘要是否相等（完整遍历全部 32 字节，无提前短路），防止时序侧信道泄露。

- **`pub fn toHex(digest: [32]u8, out: []u8): []const u8`**  
  将 32 字节摘要编码为小写十六进制 ASCII 写入 `out`（完整编码需至少 64 字节缓冲区），返回实际写入的只读切片。

## 使用示例

```typescript
import {
    Blake3Hasher,
    hash,
    keyedHash,
    deriveKey,
    hashEql,
    toHex,
} from "@cap/blake3";
import { print } from "cap:io";

pub fn main() {
    // 1. 标准一次性哈希
    const digest = hash("abc");
    let hex_buf: [64]u8 = [0; 64];
    const hex_str = toHex(digest, hex_buf[0..]);
    // 输出: 6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85
    print(hex_str);

    // 2. 流式增量更新 + XOF 64 字节扩展输出
    let hasher = Blake3Hasher.init();
    hasher.update("ab");
    hasher.update("c");
    const streamed = hasher.finalize();
    if (!hashEql(digest, streamed)) {
        @panic("BLAKE3 streaming digest mismatch");
    }

    let xof_out: [64]u8 = [0; 64];
    hasher.finalizeXof(xof_out[0..]);

    // 重置复用
    hasher.reset();
    hasher.update("hello");
    const hello_digest = hasher.finalize();
    * = hello_digest;

    // 3. 密钥哈希 (MAC)
    const key: [32]u8 = [
        'w', 'h', 'a', 't', 's', ' ', 't', 'h',
        'e', ' ', 'E', 'l', 'v', 'i', 's', 'h',
        ' ', 'w', 'o', 'r', 'd', ' ', 'f', 'o',
        'r', ' ', 'f', 'r', 'i', 'e', 'n', 'd',
    ];
    const mac = keyedHash(key, "authenticated message");
    * = mac;

    // 4. 密钥派生 (KDF)
    const session_key = deriveKey("my-app 2026-11-25 session key", "master-secret");
    * = session_key;
}
```
