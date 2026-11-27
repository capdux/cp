[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# object-inspect

> 版本：`1.13.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap debug value inspector and string/bytes/number pretty-formatter with escape and truncation support

## 安装与导入

```bash
cap add object-inspect
```

```cp
import { defaultInspectOptions } from "object-inspect";
```

## 核心导出 API

- `export fn defaultInspectOptions(): InspectOptions;`
- `export fn inspectString(s: string, opts: InspectOptions, out: []u8): usize;`
- `export fn inspectBool(v: bool, out: []u8): usize;`
- `export fn inspectInt(val: i64, out: []u8): usize;`
- `export fn inspectBytes(data: &[]u8, max_bytes: usize, out: []u8): usize;`
- `export fn inspectStringArray(items: &[]string, opts: InspectOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`object-inspect@1.13.4`)（https://github.com/inspect-js/object-inspect），许可证：MIT
