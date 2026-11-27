[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mongodb-connection-string-url

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser, option extractor, and password-redacting formatter for CP.

## 安装与导入

```bash
cap add mongodb-connection-string-url
```

```cp
import { parseMongoUri } from "mongodb-connection-string-url";
```

## 核心导出 API

- `export fn parseMongoUri(uri: string): MongoUri;`
- `export fn getQueryOption(uri: string, key: string): string;`
- `export fn formatMongoUri(cfg: MongoUri, redact_password: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mongodb-connection-string-url@1.0.0`)，许可证：Apache-2.0
- **上游生态**：`crates.io` (`mongodb@1.0.0`)，许可证：Apache-2.0
