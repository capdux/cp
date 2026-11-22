# `cap:cors_policy` — WHATWG Fetch CORS 策略与预检评估引擎

> 参考来源：/data/dev/bun/src/http_types/FetchRequestMode.cp | 吸收日期：2026-11-22

`cap:cors_policy` 提供纯 CP、零外部依赖、零堆分配的 CORS 策略配置、简单请求与预检（Preflight）评估以及响应头决策计算能力。

## 核心类型

### `CorsConfig`

```cp
pub struct CorsConfig {
    allow_all_origins: bool = false,
    allowed_origins: [16][]const u8 = [""; 16],
    origin_len: usize = 0,
    allowed_methods: [12][]const u8 = [""; 12],
    method_len: usize = 0,
    allowed_headers: [16][]const u8 = [""; 16],
    header_len: usize = 0,
    allow_credentials: bool = false,
    max_age_secs: u32? = null,

    fn addOrigin(*_, origin: []const u8): bool
    fn addMethod(*_, method: []const u8): bool
    fn addHeader(*_, header: []const u8): bool
}
```

### `CorsDecision`

```cp
pub struct CorsDecision {
    allowed: bool = false,
    allow_origin_header: []const u8 = "",
    vary_origin: bool = false,
    allow_credentials: bool = false,
    max_age_secs: u32? = null,
}
```

### `CorsPolicy`

```cp
pub struct CorsPolicy {
    config: CorsConfig = CorsConfig{},

    static fn init(config: CorsConfig): CorsPolicy
    static fn isSafelistedHeader(header_name: []const u8): bool
    fn evaluateSimple(origin: []const u8): CorsDecision
    fn evaluatePreflight(origin: []const u8, req_method: []const u8, req_headers: []const u8): CorsDecision
}
```

- **Credentials 与通配源防护**：当 `allow_credentials == true` 时，`allow_origin_header` 严禁输出 `"*"`，强制回显具体 `origin` 并置 `vary_origin = true`。
- **CORS-safelisted 请求头自动放行**：`evaluatePreflight` 自动放行 `accept`、`accept-language`、`content-language`、`content-type`（大小写无关），其余请求头按 `allowed_headers` 白名单逐项校验。

## 使用示例

```cp
import { CorsConfig, CorsPolicy } from "cap:cors_policy";

let cfg = CorsConfig{ allow_credentials: true, max_age_secs: 600 };
* = cfg.addOrigin("https://app.example.com");
* = cfg.addMethod("POST");
* = cfg.addHeader("Authorization");

const policy = CorsPolicy.init(cfg);
const decision = policy.evaluatePreflight(
    "https://app.example.com",
    "POST",
    "Content-Type, Authorization"
);
// decision.allowed == true, decision.allow_origin_header == "https://app.example.com", decision.vary_origin == true
```

## 上游行为契约与安全边界

1. **凭证模式禁止通配符（WHATWG Fetch 铁律）**：
   - 当 `CorsConfig.allow_credentials == true` 时，即使配置了 `allow_all_origins = true`，`CorsDecision.allow_origin_header` 也绝不返回 `"*"`，必须回显精确 `origin` 并设置 `vary_origin = true` 防止 CDN 跨源缓存污染。
2. **预检请求头白名单与 Safelist 豁免**：
   - `CorsPolicy.isSafelistedHeader` 大小写无关识别 `accept` / `accept-language` / `content-language` / `content-type`；非安全请求头须全部命中 `allowed_headers` 才允许通过 `evaluatePreflight`。
3. **固定容量防御**：
   - 零堆分配上限为 16 个源、12 个方法与 16 个请求头，`addOrigin`/`addMethod`/`addHeader` 超限时安全返回 `false`。
