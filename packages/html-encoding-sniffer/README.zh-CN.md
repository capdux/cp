[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# html-encoding-sniffer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation WHATWG HTML 13.2.3.2 byte stream encoding sniffer and label normalizer (npm html-encoding-sniffer parity) for CP

## 安装与导入

```bash
cap add html-encoding-sniffer
```

```cp
import { hesToLower } from "html-encoding-sniffer";
```

## 核心导出 API

- `export fn hesToLower(ch: u8): u8;`
- `export fn hesIsAsciiWs(ch: u8): bool;`
- `export fn hesTrimAsciiWs(s: string): string;`
- `export fn hesEqIgnoreCase(a: string, b: string): bool;`
- `export fn hesStartsWithIgnoreCase(bytes: &[]u8, pos: usize, needle: string): bool;`
- `export fn normalizeEncodingLabel(label: string): string;`
- `export fn hesMapMetaEncoding(label: string): string;`
- `export fn hesExtractCharsetFromContent(content: string): string;`
- `export fn hesPrescanMeta(bytes: &[]u8): string;`
- `export fn sniffHtmlEncoding(bytes: &[]u8, opts: SniffOptions): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`html-encoding-sniffer@1.0.0`)，许可证：MIT
