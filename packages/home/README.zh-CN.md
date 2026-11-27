[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# home

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 home

## 安装与导入

```bash
cap add home
```

```cp
import { isSlash } from "home";
```

## 核心导出 API

- `export fn isSlash(ch: u8): bool;`
- `export fn writeByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn trimTrailingSlashes(s: string): usize;`
- `export fn resolveHomeDir(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize;`
- `export fn expandTilde(path: string, home_dir: string, out: []u8): usize;`
- `export fn contractTilde(path: string, home_dir: string, out: []u8): usize;`
- `export fn joinConfigDir(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`home@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`untildify@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`expand-tilde@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`os-homedir@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`dirs@1.0.0`)，许可证：MIT
