# `cap:tsconfig_paths` — TSConfig `paths` 通配符映射与 `${configDir}` 展开器

> 参考来源：`/data/dev/bun/src/resolver/tsconfig_json.cp` | 吸收日期：2026-11-22

纯 CP 零堆分配 TSConfig `compilerOptions.paths` 通配符路径映射解析器与 `${configDir}` 模板展开器，吸收自 `/data/dev/bun/src/resolver/tsconfig_json.cp`。

## 导出符号

- `expandConfigDir(input, config_dir, out)`：展开所有 `${configDir}` 占位符（TypeScript 5.5+ 规范）
- `PathMapping` / `TsConfigPaths`：
  - `init(base_url)` / `addRule(pattern, targets)`（校验最多单个 `*` 通配符）
  - `resolve(specifier, out)` / `resolveNth(specifier, idx, out)`：严格遵循 TypeScript 模块解析优先级（精确匹配优先 -> 最长通配符前缀匹配 -> `*` 替换与 `baseUrl` 拼接）

## 上游行为契约与边界注意事项

1. **TypeScript 模块解析优先级**：
   - 无通配符的精确匹配规则永远优先于含 `*` 的通配符规则；
   - 多个通配符规则同时匹配时，严格选取**通配符前缀最长**的规则（如 `@app/ui/*` 优先于 `@app/*`），避免别名阴影。
2. **多候选目标回退（`resolveNth`）**：
   - 一条 `paths` 规则可配置多个候选目标路径，调用方应通过 `resolveNth(specifier, idx, out)` 按索引依次尝试并结合文件系统存在性校验实现完整回退。
3. **非法通配符与缓冲边界防御**：
   - `addRule` 拒绝包含超过 1 个 `*` 通配符的非法模式；
   - `expandConfigDir` 与 `resolve*` 全程零堆分配，当调用方提供的 `out` 缓冲区不足时安全返回 `null`，绝不越界写入。
