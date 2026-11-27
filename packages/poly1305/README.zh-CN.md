[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# poly1305

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator (Donna 26-bit limbs + constant-time verification via subtle)

## 安装与导入

```bash
cap add poly1305
```

```cp
import { poly1305Mac } from "poly1305";
```

## 核心导出 API

- `export fn poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool;`
- `export fn poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`poly1305@1.0.0`)，许可证：MIT
