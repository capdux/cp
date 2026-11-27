[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/resolv-conf

Zero-heap POSIX `/etc/resolv.conf` DNS resolver configuration parser in pure CP.

## API

- `defaultResolvConfig(): ResolvConfig`
- `parseResolvConf(content: string): ResolvConfig` — parses `nameserver` (IPv4/IPv6), `search`, `domain`, and `options` (`ndots`, `timeout`, `attempts`, `rotate`, `edns0`, `use-vc`, `trust-ad`), ignoring `#` and `;` comments.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`resolv-conf@0.7.0`)
- **Upstream Repository**: https://github.com/tailhook/resolv-conf
- **License**: MIT (clean-room CP implementation)
