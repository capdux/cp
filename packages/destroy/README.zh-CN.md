[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# destroy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap safe idempotent stream and file-descriptor destroyer with lifecycle completion guard (npm destroy equivalent, reuses on-finished)

## 安装与导入

```bash
cap add destroy
```

```cp
import { destroyStream } from "destroy";
```

## 核心导出 API

- `export fn destroyStream(s: &mut DestroyableStream): u8;`
- `export fn destroyWithError(s: &mut DestroyableStream, err_code: i32): u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`destroy@1.0.0`)，许可证：MIT
