[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# send

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP static file transfer planner with traversal protection, dotfiles policy, ETag/304, Range/206, and redirect URL encoding (clean-room MIT implementation of npm send v1)

## 安装与导入

```bash
cap add send
```

```cp
import { planSendFile } from "send";
```

## 核心导出 API

- `export fn planSendFile(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan;`
- `export fn formatRedirectUrl(path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`send@1.0.0`)，许可证：MIT
