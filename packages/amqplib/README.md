[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `amqplib`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amqplib` ([https://github.com/amqp-node/amqplib](https://github.com/amqp-node/amqplib)) / `crates.io:lapin` ([https://github.com/amqp-rs/lapin](https://github.com/amqp-rs/lapin))（原库名 `amqplib` → CP 包名 `amqplib`）

Zero-heap-allocation AMQP 0-9-1 / RabbitMQ connection URI parser and log-redacting formatter in pure CP.

## Features

- **`parseAmqpUrl`**: Parses `amqp://` (default port `5672`) and `amqps://` (default port `5671`), default `guest:guest` credentials, `%2F` root vhost, IPv6 `[::1]:5672`, and `?heartbeat=<secs>`.
- **`formatAmqpUrl`**: Serializes AMQP URIs with optional password redaction (`redact_password: true` -> `***`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { AmqpConfig, parseAmqpUrl, formatAmqpUrl } from "amqplib";
```

## License

MIT
