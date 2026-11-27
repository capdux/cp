[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# magnet-url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 btih & btmh) parser, tracker extractor, and builder for CP (compatible with npm magnet-uri & Rust magnet-url)

## 安装与导入

```bash
cap add magnet-url
```

```cp
import { toLower } from "magnet-url";
```

## 核心导出 API

- `export fn toLower(c: u8): u8;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn startsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn isHexStr(s: string): bool;`
- `export fn isBase32Str(s: string): bool;`
- `export fn parsePositiveI64(s: string): i64;`
- `export fn classifyExactTopic(xt: string): TopicMatch;`
- `export fn parseMagnet(uri: string): MagnetLink;`
- `export fn appendBytes(dst: []u8, pos: usize, src: string): usize;`
- `export fn formatMagnetV1(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`magnet-url@1.0.0`)，许可证：MIT
