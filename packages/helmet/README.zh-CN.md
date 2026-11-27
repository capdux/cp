[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# helmet

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP security response headers policy generator (CSP, HSTS, COOP, CORP, X-Frame-Options, Referrer-Policy) with CRLF-injection defense in pure CP (clean-room MIT counterpart to npm helmet v8).

## 安装与导入

```bash
cap add helmet
```

```cp
import { hlmWriteByte } from "helmet";
```

## 核心导出 API

- `export fn hlmWriteByte(out: []u8, pos: &mut usize, b: u8);`
- `export fn hlmWriteStr(out: []u8, pos: &mut usize, s: string);`
- `export fn hlmWriteU64(out: []u8, pos: &mut usize, val: u64);`
- `export fn hlmWriteHeaderLine(out: []u8, pos: &mut usize, name: string, val: string);`
- `export fn hlmAppendCspDirective(out: []u8, pos: &mut usize, has_any: &mut bool, dir_name: string, val: string);`
- `export fn isSafeHeaderValue(val: string): bool;`
- `export fn formatHstsHeader(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize;`
- `export fn formatCspHeader(default_src: string, script_src: string, style_src: string, img_src: string, connect_src: string, out: []u8): usize;`
- `export fn formatHelmetHeaders(opts: HelmetOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`helmet@1.0.0`)，许可证：MIT
