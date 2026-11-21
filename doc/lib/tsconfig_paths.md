# `cap:tsconfig_paths`

纯 CP 零堆分配 TSConfig `compilerOptions.paths` 通配符路径映射解析器与 `${configDir}` 模板展开器，吸收自 `/data/dev/bun/src/resolver/tsconfig_json.cp`。

## 导出符号

- `expandConfigDir(input, config_dir, out)`：展开所有 `${configDir}` 占位符
- `PathMapping` / `TsConfigPaths`：
  - `init(base_url)` / `addRule(pattern, targets)`（校验最多单个 `*` 通配符）
  - `resolve(specifier, out)` / `resolveNth(specifier, idx, out)`：严格遵循 TypeScript 模块解析优先级（精确匹配优先 -> 最长通配符前缀匹配 -> `*` 替换与 `baseUrl` 拼接）
