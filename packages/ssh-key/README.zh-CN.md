[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ssh-key

Zero-heap OpenSSH public key (ssh-ed25519, ssh-rsa, ecdsa-sha2-*, sk-ssh-*) authorized_keys parser, validator, and formatter for CP (compatible with npm sshpk & Rust ssh-key)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ssh-key` ([https://github.com/RustCrypto/SSH](https://github.com/RustCrypto/SSH)) / `npm:sshpk` ([https://github.com/TritonDataCenter/node-sshpk](https://github.com/TritonDataCenter/node-sshpk))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ssh-key` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ssh-key
```

```cp
import {
    SshPublicKey,
    isHspace,
    identifyAlgorithm,
    isValidBase64,
    parsePublicKey,
    formatPublicKey,
    countAuthorizedKeys,
    isModernAlgorithm,
} from "ssh-key";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SshPublicKey` | `class` | 字段: `alg: u8, key_type: string, key_b64: string, comment: string, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isHspace` | `(c: u8): bool` | 执行 `isHspace` 条件判定，返回 `bool`。 |
| `identifyAlgorithm` | `(key_type: string): u8` | 调用 `identifyAlgorithm` 并返回 `u8`。 |
| `isValidBase64` | `(b64: string): bool` | 执行 `isValidBase64` 条件判定，返回 `bool`。 |
| `parsePublicKey` | `(line: string): SshPublicKey` | 通过 `parsePublicKey` 解析输入数据，返回 `SshPublicKey`。 |
| `formatPublicKey` | `(key: SshPublicKey, out: []u8): usize` | 通过 `formatPublicKey` 编码或格式化输出，返回 `usize`。 |
| `countAuthorizedKeys` | `(text: string): usize` | 调用 `countAuthorizedKeys` 并返回 `usize`。 |
| `isModernAlgorithm` | `(alg: u8): bool` | 执行 `isModernAlgorithm` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidBase64 } from "ssh-key";

pub fn main() {
    let result = isValidBase64("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
