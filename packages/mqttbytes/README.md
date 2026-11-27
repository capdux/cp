# @cap/mqtt-packet

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mqttbytes` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `mqttbytes` → CP 包名 `@cap/mqtt-packet`）

Zero-heap OASIS MQTT v3.1.1 binary control packet encoder and streaming decoder in pure CP.

## Features

- `encodeRemainingLen` & `decodeHeader` supporting 1..4-byte variable-length encoding (`0..268435455`)
- `encodePublish` & zero-copy `decodePublish` (QoS 0/1/2, `retain`, `dup`, `packet_id`)
- `encodeSimple` (`PINGREQ`, `PINGRESP`, `DISCONNECT`) and `encodePuback`
- Streaming partial-frame detection (`MQTT_INCOMPLETE` vs `MQTT_INVALID`)

## Usage

```cp
import { encodePublish, decodePublish } from "@cap/mqtt-packet";

let buf: [128]u8 = undefined;
const n = encodePublish("sensors/temp", "23.5", 1, true, 42, buf[0..128]);
const pub_pkt = decodePublish(buf[0..(n as usize)]);
```
