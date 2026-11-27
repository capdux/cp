[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# serve-static

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP static file request resolver, dotfile policy evaluator, Cache-Control builder, and MIME/ETag helper (clean-room port of npm serve-static)

## 安装与导入

```bash
cap add serve-static
```

```cp
import { sstPush } from "serve-static";
```

## 核心导出 API

- `export fn sstPush(out: []u8, pos: usize, s: string): usize;`
- `export fn sstIsGetOrHead(method: string): bool;`
- `export fn hasDotfileSegment(pathname: string): bool;`
- `export fn resolveStaticRequest(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution;`
- `export fn formatCacheControl(max_age_secs: u64, immutable: bool, out: []u8): usize;`
- `export fn formatStaticEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize;`
- `export fn formatStaticContentType(file_path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`serve-static@1.0.0`)，许可证：MIT
