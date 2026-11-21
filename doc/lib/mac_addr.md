# `cap:mac_addr` — IEEE 802 MAC-48 / EUI-48 Address Parser & EUI-64 Converter

Zero-allocation MAC-48 hardware address parser supporting colon, hyphen, Cisco dot, and bare hex notations, unicast/multicast/broadcast/LAA classification, canonical formatting, and RFC 4291 Modified EUI-64 derivation.

## Exports

- `MacAddr` — `parse(raw): MacAddr?`, `isZero()`, `isBroadcast()`, `isMulticast()`, `isUnicast()`, `isLocallyAdministered()`, `toEui64(): [8]u8`, `formatColon(out)`, `formatHyphenUpper(out)`.
