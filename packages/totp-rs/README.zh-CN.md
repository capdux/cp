[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# totp-rs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Google Authenticator otpauth:// TOTP/HOTP Key URI parser, Base32 secret validator, and builder for CP.

## 安装与导入

```bash
cap add totp-rs
```

```cp
import { isValidBase32Secret } from "totp-rs";
```

## 核心导出 API

- `export fn isValidBase32Secret(secret: string): bool;`
- `export fn parseOtpAuthUri(uri: string): OtpAuthUri;`
- `export fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize;`
- `export fn splitLabel(label: string): LabelParts;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`totp-rs@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`otpauth@1.0.0`)，许可证：MIT
