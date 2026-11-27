<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`decamelize`](https://github.com/sindresorhus/decamelize)（`MIT`）（包名 `@cap/decamelize`）。

# @cap/decamelize

零堆分配的 CP 原生驼峰转分隔符字符串库，Clean-Room MIT 实现对标 npm 顶流包 `decamelize`（1.2 亿+/周下载，`yargs` / `meow` 核心依赖）。

## 特性

- `decamelize(input, separator, out)`：自定义分隔符转换（如 `"unicornRainbow"` → `"unicorn_rainbow"`）
- `decamelizeWithOptions`：支持 `preserveConsecutiveUppercase` 保留连续大写缩写词（如 `"testURL"` → `"test_URL"`）
- `toSnakeCase` / `toKebabCase` 快捷函数

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`sindresorhus/decamelize`](https://github.com/sindresorhus/decamelize)。
