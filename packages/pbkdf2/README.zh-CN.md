[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pbkdf2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 2898 / RFC 8018 PBKDF2-HMAC-SHA256 password-based key derivation and constant-time verification

## 安装与导入

```bash
cap add pbkdf2
```

```cp
import { pbkdf2HmacSha256 } from "pbkdf2";
```

## 核心导出 API

- `export fn pbkdf2HmacSha256(password: []u8, salt: []u8, iterations: u32, out_dk: []u8): bool;`
- `export fn pbkdf2Verify(password: []u8, salt: []u8, iterations: u32, expected_dk: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pbkdf2@1.0.0`)，许可证：MIT OR Apache-2.0
