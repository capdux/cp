# `@cap/amqp-url`

Zero-heap-allocation AMQP 0-9-1 / RabbitMQ connection URI parser and log-redacting formatter in pure CP.

## Features

- **`parseAmqpUrl`**: Parses `amqp://` (default port `5672`) and `amqps://` (default port `5671`), default `guest:guest` credentials, `%2F` root vhost, IPv6 `[::1]:5672`, and `?heartbeat=<secs>`.
- **`formatAmqpUrl`**: Serializes AMQP URIs with optional password redaction (`redact_password: true` -> `***`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { AmqpConfig, parseAmqpUrl, formatAmqpUrl } from "@cap/amqp-url";
```

## License

MIT
