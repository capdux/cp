[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# read-pkg

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation package.json / cap.json parser and normalizer for CP, reusing normalize-package-data.

## 安装与导入

```bash
cap add read-pkg
```

```cp
import { rpkgIsSpace } from "read-pkg";
```

## 核心导出 API

- `export fn rpkgIsSpace(ch: u8): bool;`
- `export fn rpkgStrEq(a: string, b: string): bool;`
- `export fn rpkgStartsWithAt(s: string, pos: usize, kw: string): bool;`
- `export fn rpkgIsValueBoundary(s: string, pos: usize): bool;`
- `export fn rpkgSkipWs(s: string, pos: usize): usize;`
- `export fn rpkgFindStringEnd(s: string, open_quote: usize): usize?;`
- `export fn rpkgSkipValue(s: string, pos: usize): usize?;`
- `export fn rpkgValidateTopObject(json_text: string): bool;`
- `export fn rpkgFindTopLevelValue(json_text: string, key: string): usize?;`
- `export fn extractJsonStringField(json_text: string, key: string): string?;`
- `export fn extractJsonBoolField(json_text: string, key: string): bool?;`
- `export fn rpkgExtractRepository(json_text: string): string;`
- `export fn parsePackageJson(json_text: string, normalize: bool): ReadPkgResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`read-pkg@1.0.0`)，许可证：MIT
