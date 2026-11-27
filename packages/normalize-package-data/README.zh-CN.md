[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# normalize-package-data

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation package metadata normalizer and validator for CP, reusing @cap/validate-npm-package-name and @cap/validate-npm-package-license.

## 安装与导入

```bash
cap add normalize-package-data
```

```cp
import { npdIsSpace } from "normalize-package-data";
```

## 核心导出 API

- `export fn npdIsSpace(ch: u8): bool;`
- `export fn npdTrim(s: string): string;`
- `export fn npdStartsWith(s: string, prefix: string): bool;`
- `export fn npdEndsWith(s: string, suffix: string): bool;`
- `export fn npdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn npdIsOwnerRepoShorthand(s: string): bool;`
- `export fn npdWriteHostedUrl(base_url: string, slug: string, out: []u8): usize;`
- `export fn expandRepoShorthand(repo: string, out: []u8): usize;`
- `export fn normalizeEntryPath(entry: string): string;`
- `export fn cleanVersionString(ver: string): string;`
- `export fn normalizePackageData(meta: PackageMetadata): NormalizedPackageResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`normalize-package-data@1.0.0`)，许可证：BSD-2-Clause
