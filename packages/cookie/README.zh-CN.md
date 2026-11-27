[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cookie

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Rust-style CookieBuilder, percent-encoded Set-Cookie helpers, and removal cookie generator for CP.

## 安装与导入

```bash
cap add cookie
```

```cp
import { ckcDefaultCookie } from "cookie";
```

## 核心导出 API

- `export fn ckcDefaultCookie(name: string, value: string): Cookie;`
- `export fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta;`
- `export fn formatEncodedCookie(c: Cookie, out: []u8): usize;`
- `export fn makeRemovalCookie(name: string, path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cookie@1.0.0`)，许可证：MIT
