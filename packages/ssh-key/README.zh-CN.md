[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ssh-key

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap OpenSSH public key (ssh-ed25519, ssh-rsa, ecdsa-sha2-*, sk-ssh-*) authorized_keys parser, validator, and formatter for CP (compatible with npm sshpk & Rust ssh-key)

## 安装与导入

```bash
cap add ssh-key
```

```cp
import { isSpace } from "ssh-key";
```

## 核心导出 API

- `export fn isSpace(c: u8): bool;`
- `export fn isLineTrim(c: u8): bool;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn identifyAlgorithm(key_type: string): u8;`
- `export fn isValidBase64(b64: string): bool;`
- `export fn parsePublicKey(line: string): SshPublicKey;`
- `export fn formatPublicKey(key: SshPublicKey, out: []u8): usize;`
- `export fn countAuthorizedKeys(text: string): usize;`
- `export fn isModernAlgorithm(alg: u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ssh-key@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`sshpk@1.0.0`)，许可证：MIT
