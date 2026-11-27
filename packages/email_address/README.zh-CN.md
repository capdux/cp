[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# email_address

> 版本：`0.2.9` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 5321/5322 email address parser and validator

## 安装与导入

```bash
cap add email_address
```

```cp
import { parseEmail } from "email_address";
```

## 核心导出 API

- `export fn parseEmail(input: string): EmailAddress;`
- `export fn isValidEmail(input: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`email_address@0.2.9`)（https://github.com/johnstonskj/rust-email_address），许可证：MIT
