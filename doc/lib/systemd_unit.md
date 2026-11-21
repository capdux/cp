# `cap:systemd_unit` — Linux systemd 单元文件零分配解析库

> 来源：吸收自 `/data/dev/bun/src/ini.zig`（Linux systemd Unit File Parser）

`cap:systemd_unit` 提供纯 CP、零外部依赖、零堆分配的 Linux systemd `.service`/`.timer`/`.socket` 单元文件流式解析、续行拼接、重复键统计与覆盖/清空语义查询能力。

## 核心类型与函数

### `UnitEntry` & `UnitIter`

```cp
pub struct UnitEntry {
    section: []const u8 = "",
    key: []const u8 = "",
    value: []const u8 = "",
    line_no: usize = 0,
}

pub struct UnitIter {
    static fn init(content: []const u8): UnitIter
    fn next(*_, val_buf: []u8): UnitEntry?
}
```

- 自动剥离 UTF-8 BOM、跳过空行与 `#`/`;` 注释行、识别 `[Unit]`/`[Service]`/`[Install]` 等段头。
- 遵循 `systemd.syntax(7)` 行尾 `\` 续行规则（跳过续行间的 `#`/`;` 注释行，以单空格拼接），按原始顺序完整保留重复键并追踪 1-based `line_no`。

### 查询与统计函数

```cp
pub fn getUnitValue(content: []const u8, section: []const u8, key: []const u8, val_buf: []u8): ([]const u8)?
pub fn countSectionKey(content: []const u8, section: []const u8, key: []const u8): usize
```

- `getUnitValue`：按 systemd 语义返回最终生效值（后者覆盖前者，空赋值 `Key=` 清空先前值）。
- `countSectionKey`：统计指定段下重复键（如 `Environment`、`ExecStartPre`）的出现次数。

## 使用示例

```cp
import { UnitIter, getUnitValue, countSectionKey } from "cap:systemd_unit";

const svc = "[Service]\nExecStart=/usr/bin/cap \\\n  run app.cp\nEnvironment=A=1\nEnvironment=B=2\n";
let buf: [256]u8 = [0; 256];
const exec = getUnitValue(svc, "Service", "ExecStart", buf[0..]).?; // "/usr/bin/cap run app.cp"
// countSectionKey(svc, "Service", "Environment") == 2
```
