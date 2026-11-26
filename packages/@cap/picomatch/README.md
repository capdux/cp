# @cap/picomatch

Zero-allocation glob matching engine & static base scanner for CP — clean-room MIT parity with npm `picomatch` v4. Reuses `@cap/normalize-path` and `@cap/is-extglob`.

纯 CP 零堆分配高频通配符匹配引擎（对标 npm `picomatch` v4），完整支持 `*`/`?`/`**` globstar、字符类 `[a-z]`/`[!0-9]`、嵌套花括号 `{a,b,c}`、前导 `!`/`!!` 取反、点文件保护、`nocase`/`basename`/`noglobstar` 选项、组合规则过滤与 `scanBase` 静态前缀提取。

## Install / 安装

```bash
cap add @cap/picomatch
```

## Dependencies / 跨包复用

- `@cap/normalize-path`: Windows 反斜杠与路径分隔符归一化
- `@cap/is-extglob`: Bash/Micromatch extglob 语法检测

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PicoOptions` | `struct { dot, nocase, basename, noglobstar: bool }` | Matcher options with `PicoOptions.defaults()` |
| `isMatch` | `(path: string, pattern: string): bool` | Match `path` against `pattern` with default options |
| `isMatchWithOptions` | `(path: string, pattern: string, opts: PicoOptions): bool` | Match `path` with custom `PicoOptions` |
| `isMatchAny` | `(path: string, patterns: []const string, opts: PicoOptions): bool` | Match against combined positive + negative (`!`) glob rules |
| `scanBase` | `(pattern: string, out: []u8): usize` | Extract static non-glob base prefix (e.g. `"src/lib/**/*.cp"` -> `"src/lib"`) |

## Usage / 使用示例

```cp
import { PicoOptions, isMatch, isMatchWithOptions, isMatchAny, scanBase } from "@cap/picomatch";

const ok = isMatch("src/lib/router.cp", "src/**/*.{ts,cp,zig}"); // true

let opts = PicoOptions.defaults();
opts.dot = true;
const hidden = isMatchWithOptions(".env", "*", opts); // true

let baseBuf: [64]u8 = undefined;
const n = scanBase("src/lib/**/*.cp", baseBuf[0..64]); // "src/lib"
```
