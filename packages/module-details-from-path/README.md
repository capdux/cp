[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/module-details-from-path

Zero-heap `node_modules` / `cap_modules` path parser in pure CP supporting scoped packages (`@scope/name`), nested module directories, and POSIX/Windows separators.

## API

- `ModuleDetails { name, basedir, subpath, found }`
- `parseModulePath(file_path): ModuleDetails`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`module-details-from-path@1.0.4`)
- **Upstream Repository**: https://github.com/watson/module-details-from-path
- **License**: MIT (clean-room CP implementation)
