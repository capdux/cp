# `cap:dockerfile_parse` — Dockerfile 指令流式解析器

> 来源：吸收自 `/data/dev/bun/src/cli.zig`（Dockerfile Instruction Parser）

`cap:dockerfile_parse` 提供纯 CP、零外部依赖、零堆分配的 Dockerfile 流式指令解析能力，支持 `# escape=\`` 指令、续行折叠、注释与空行剥离、1-based 行号追踪以及 `FROM` / `EXPOSE` 专用参数解析器。

## 核心类型与函数

### `DockerInstruction` & `DockerfileIter`

```cp
pub struct DockerInstruction {
    keyword: []const u8 = "",
    args: []const u8 = "",
    line_no: usize = 0,
}

pub struct DockerfileIter {
    static fn init(src: []const u8): DockerfileIter
    fn next(*_, args_buf: []u8): DockerInstruction?
}
```

- 自动剥离 UTF-8 BOM、跳过空行与 `#` 注释行。
- 在文件顶部识别 `# escape=\`` 或 `# escape=\` 解析器指令，支持跨多行续行折叠（自动忽略续行之间的注释与空行）并精确记录指令起始 1-based 行号 `line_no`。

### `FromSpec` & `parseFromArgs`

```cp
pub struct FromSpec {
    platform: ([]const u8)? = null,
    image: []const u8 = "",
    stage_alias: ([]const u8)? = null,
}

pub fn parseFromArgs(args: []const u8): FromSpec?
```

- 解析如 `--platform=linux/amd64 node:22-alpine AS builder`、`alpine:3.20` 等 `FROM` 参数。

### `ExposePort` & `parseExposeToken`

```cp
pub struct ExposePort {
    port: u16 = 0,
    is_udp: bool = false,
}

pub fn parseExposeToken(tok: []const u8): ExposePort?
```

- 解析 `8080`、`8080/tcp`、`53/udp`（拒绝端口 0、>65535 与未知协议）。

## 使用示例

```cp
import { DockerfileIter, parseFromArgs, parseExposeToken } from "cap:dockerfile_parse";

const df = "FROM --platform=linux/amd64 node:22-alpine AS builder\nEXPOSE 8080/tcp\n";
let it = DockerfileIter.init(df);
let buf: [256]u8 = [0; 256];
const i1 = it.next(buf[0..]).?; // keyword="FROM", line_no=1
const from_spec = parseFromArgs(i1.args).?; // platform="linux/amd64", image="node:22-alpine", stage_alias="builder"
```
