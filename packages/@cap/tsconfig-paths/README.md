# @cap/tsconfig-paths

Zero-allocation TypeScript `tsconfig.json` `baseUrl` + `paths` alias wildcard matcher and path resolver for CP — clean-room MIT parity with npm `tsconfig-paths`.

零堆分配的 CP `tsconfig.json` 路径别名匹配与展开引擎，严格遵循 TypeScript 官方优先级规范（精确匹配 > 最长静态前缀通配符 > 通用 `*`），支持多目标回退候选展开与路径规范化拼接（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/tsconfig-paths
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `PathRule` / `PathsMatcher` | `pub struct` | 规则与匹配器（提供 `PathsMatcher.init(base_url)`、`addRule(*_, pattern, target)`、`addRuleMulti(*_, pattern, t1, t2)`） |
| `PatternMatch` | `pub struct { matched, is_exact, prefix_len, captured }` | 模式匹配详情 |
| `CandidateList` | `pub struct { items: [4]string, count: usize }` | 多目标回退候选列表（提供 `itemAt(i)`） |
| `matchesPattern` | `(pattern: string, specifier: string) -> PatternMatch` | 匹配精确或单 `*` 通配模式并捕获子串 |
| `resolvePath` | `(matcher: PathsMatcher, specifier: string, out: []u8) -> string?` | 解析首选规范化路径，未命中返回 `null` |
| `resolveAllCandidates` | `(matcher: PathsMatcher, specifier: string, out_buf: []u8) -> CandidateList` | 展开胜出规则的全部多目标回退路径 |

## Usage / 使用示例

```cp
import { PathsMatcher, resolvePath, resolveAllCandidates } from "@cap/tsconfig-paths";

let m = PathsMatcher.init("./src");
m.addRule("@app/config", "config/prod.ts");
m.addRule("@app/*", "app/*");
m.addRuleMulti("@ui/*", "components/*", "../shared/ui/*");

let out: [256]u8 = undefined;
const p1 = resolvePath(m, "@app/config", out[0..256]).?; // "src/config/prod.ts"
const p2 = resolvePath(m, "@app/auth/jwt", out[0..256]).?; // "src/app/auth/jwt"

let cbuf: [512]u8 = undefined;
const cands = resolveAllCandidates(m, "@ui/Button", cbuf[0..512]);
// cands.itemAt(0) => "src/components/Button"
// cands.itemAt(1) => "shared/ui/Button"
```

## License

MIT
