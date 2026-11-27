[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-fifo

> 版本：`1.3.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap O(1) ring-buffer FIFO queue for i64 and string items

## 安装与导入

```bash
cap add fast-fifo
```

```cp
import { newFifoI64 } from "fast-fifo";
```

## 核心导出 API

- `export fn newFifoI64(): FifoI64;`
- `export fn newFifoStr(): FifoStr;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fast-fifo@1.3.2`)（https://github.com/mafintosh/fast-fifo），许可证：MIT
