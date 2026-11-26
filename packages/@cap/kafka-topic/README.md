# `@cap/kafka-topic`

Zero-heap-allocation Apache Kafka / Redpanda topic validator, official Murmur2 `DefaultPartitioner` hash, and Topic-Partition parser in pure CP.

## Features

- **`isValidTopicName` & `hasMetricCollisionRisk`**: Enforces Kafka's 1..249 character topic naming rules and flags `.`+`_` metric name collisions.
- **`murmur2`, `toPositive`, `partitionForKey`**: Bit-exact match to Apache Kafka `Utils.murmur2` (seed `0x9747b28c`) and `DefaultPartitioner`.
- **`parseTopicPartition`**: Zero-copy parser for `"orders.events:12"` and `"orders.events-12"` specs.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    TopicPartition,
    isValidTopicName,
    hasMetricCollisionRisk,
    murmur2,
    toPositive,
    partitionForKey,
    parseTopicPartition,
} from "@cap/kafka-topic";
```

## License

MIT
