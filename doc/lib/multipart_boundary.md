# `cap:multipart_boundary` — RFC 7578 `multipart/form-data` 流式解析器

> 来源：吸收自 `/data/dev/bun/src/http/HTTPRequestBody.cp`（RFC 7578 `multipart/form-data` 规范）

`cap:multipart_boundary` 提供纯 CP、零外部依赖、零堆分配的 `multipart/form-data` 边界校验与流式分片解析能力，零拷贝切分每个分片的头部区与二进制/文本 body 区。

## 核心函数与类型

### `isValidBoundary`

```cp
pub fn isValidBoundary(boundary: []const u8): bool
```

- 按 RFC 2046 §5.1.1 校验边界合法性：长度 `1..=70`、仅含合法字符集（字母/数字及 `'()+_,-./:=? `）、尾字符不得为空格。

### `MultipartPart`

```cp
pub struct MultipartPart {
    raw_headers: []const u8 = "",
    body: []const u8 = "",

    fn getHeader(header_name: []const u8): ([]const u8)?
}
```

- `getHeader(header_name)`：大小写无关查找分片内部头部（如 `Content-Disposition`、`Content-Type`），返回裁剪 OWS 后的零拷贝值切片。

### `MultipartParser`

```cp
pub struct MultipartParser {
    static fn init(body: []const u8, boundary: []const u8): MultipartParser
    fn next(*_): MultipartPart?
    fn isFinished(): bool
}
```

- 自动跳过前导 preamble 与尾随 epilogue，同时兼容 `\r\n` 与 `\n` 换行。
- 精确剥离紧邻下一个 `--<boundary>` 之前的唯一换行符，完整保留 body 内部自身换行与二进制字节（含 `\x00`）。
- 仅当正常遇到终止边界 `--<boundary>--` 时 `isFinished()` 返回 `true`；若输入截断或缺失终止边界，`isFinished()` 保持 `false`。

## 使用示例

```cp
import { MultipartParser, isValidBoundary } from "cap:multipart_boundary";

const body = "--bnd\r\nContent-Disposition: form-data; name=\"user\"\r\n\r\nalice\r\n--bnd--\r\n";
let parser = MultipartParser.init(body, "bnd");
while (part := parser.next()) {
    const disp = part.getHeader("content-disposition");
    // part.body == "alice"
}
// parser.isFinished() == true
```
