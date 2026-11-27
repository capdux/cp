[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# finalhandler

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP final response & error handler with XSS-safe HTML formatting and security headers (clean-room port of npm finalhandler)

## 安装与导入

```bash
cap add finalhandler
```

```cp
import { fhPush } from "finalhandler";
```

## 核心导出 API

- `export fn fhPush(out: []u8, pos: usize, s: string): usize;`
- `export fn fhPushU64(out: []u8, pos: usize, v: u64): usize;`
- `export fn normalizeErrorStatus(code: u16): u16;`
- `export fn statusReason(code: u16): string;`
- `export fn formatErrorHtml(message: string, out: []u8): usize;`
- `export fn formatCannotMessage(method: string, url: string, out: []u8): usize;`
- `export fn buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`finalhandler@1.0.0`)，许可证：MIT
