[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# is-binary-path

Zero-allocation check if a file path has a known binary file extension (images, audio/video, archives, native binaries, fonts, documents, ML weights, databases).

## API

- `isBinaryPath(file_path: string): bool` — returns `true` if `file_path` has a known binary extension (case-insensitive, handles POSIX `/` and Windows `\`, ignores dotfiles like `.png`).
- `isBinaryExtension(ext: string): bool` — checks a bare extension string (without leading dot).
- `extractExtension(file_path: string): string` — extracts the extension slice from a path.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`is-binary-path@3.0.0`)
- **Upstream Repository**: https://github.com/sindresorhus/is-binary-path
- **License**: MIT (clean-room CP implementation)
