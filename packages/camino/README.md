[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `camino`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:camino` ([https://github.com/camino-rs/camino](https://github.com/camino-rs/camino))

Zero-heap, strictly validated UTF-8 path engine (`Utf8PathBuf`) for CP — clean-room MIT equivalent of crates.io `camino`.

## Features

- Strict RFC 3629 UTF-8 byte validation via `@cap/utf8-valid` (`Utf8PathBuf.fromBytes`)
- Absolute (`/`) and relative path manipulation (`push`, `pop`, `setFileName`, `setExtension`)
- Component-aware queries (`parent`, `fileNameInto`, `fileStemInto`, `extensionInto`, `hasPrefixPath`, `stripPrefix`, `componentCount`)
- Zero heap allocation using fixed stack buffer (`CAMINO_MAX_PATH = 512`)
