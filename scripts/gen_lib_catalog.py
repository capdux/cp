#!/usr/bin/env python3
"""Generate domain-categorized catalog /data/dev/cp/doc/lib/CATALOG.md for all CP stdlib modules."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path("/data/dev/cp")
LIB_ROOT = REPO_ROOT / "lib"
DOC_ROOT = REPO_ROOT / "doc" / "lib"
OUT_PATH = DOC_ROOT / "CATALOG.md"

BUN_SRC_RE = re.compile(r"//\s*Source:\s*Absorbed from\s+/data/dev/bun/src/[^\s(|]+")
PURE_CP_RE = re.compile(r"//\s*Origin:\s*Pure-CP Stdlib")
PAREN_DESC_RE = re.compile(r"\(([^()]+)\)")

# Ordered list of (category_id, zh_title, en_title, name_tokens, desc_keywords)
CATEGORIES: list[tuple[str, str, str, set[str], list[str]]] = [
    (
        "runtime",
        "1. 核心运行时、并发与系统原语",
        "Core Runtime, Concurrency & OS",
        {
            "_cap", "actor", "aio", "beam", "chrono", "circuit_breaker", "conf", "cron",
            "di", "dotenv", "env", "event", "event_loop", "ffi", "fs", "fsm", "health",
            "io", "mem", "meta", "napi", "parallel", "pipeline", "pool", "proc", "process",
            "ptr", "rate_limit", "ratelimit", "reflect", "retry", "safety", "scheduler",
            "signal", "stream", "sync", "sys", "task", "time", "timer", "watchdog",
            "watcher", "worker", "lock", "mutex", "channel", "semaphore", "barrier",
            "thread", "fiber", "coro", "arena", "slab", "bump", "alloc", "os", "posix",
            "rlimit", "cgroup", "namespace", "seccomp", "sandbox", "plugin", "hook",
            "lifecycle", "shutdown", "supervisor", "backoff", "bulkhead", "timeout",
            "debounce", "throttle", "clock", "tz", "timezone", "date", "calendar",
            "duration", "stopwatch", "feature_flag", "config", "settings", "envfile",
            "bit_ops", "crash_handler", "errno", "feature", "padding_checker", "simd",
            "type_traits", "rule",
        },
        [
            "actor", "beam", "event loop", "concurrency", "mutex", "rwlock", "atomic",
            "allocator", "memory pool", "watchdog", "file watcher", "inotify", "ffi",
            "napi", "scheduler", "circuit breaker", "retry", "rate limit", "cron",
            "state machine", "dependency injection", "health probe", "process", "signal",
            "filesystem", "stream", "parallel", "worker", "cow slice", "thread",
        ],
    ),
    (
        "collections",
        "2. 基础数据结构、容器与经典算法",
        "Collections, Data Structures & Algorithms",
        {
            "aho", "ahocorasick", "bitset", "bloom", "bloom_cuckoo", "btree", "cms",
            "col", "collections", "countmin", "crdt", "cuckoo", "diff", "dsu", "fenwick",
            "fuzzy", "gap_buffer", "glob", "heap", "hyperloglog", "hll", "intrusive_heap",
            "kmp", "lfu", "lru", "merkle", "myers", "myers_diff", "patch", "pattern",
            "piece_table", "radix", "regex", "ring", "ringbuf", "roaring", "rope",
            "search", "segtree", "skiplist", "sort", "suffix", "tdigest", "trie",
            "unionfind", "xorfilter", "zset", "treap", "avl", "rbtree", "splay", "bplus",
            "deque", "list", "vec", "hashmap", "hashset", "ordered_map", "multimap",
            "interval_tree", "wavelet_tree", "fmindex", "burrows", "dawg", "fst",
            "levenshtein", "edit_distance", "simhash", "minhash", "lsh", "consistent_hash",
            "rendezvous", "jump_hash", "reservoir", "sketch", "quantile", "histogram",
            "cache", "arc_cache", "sieve", "tinylfu", "slotmap", "generational",
            "sparse_set", "bitvec", "succinct", "rank_select", "elias_fano", "patricia",
            "art", "critbit", "hamt", "rrb", "finger_tree", "persistent",
            "bptree", "exact_matcher", "pqueue", "hive_array",
        },
        [
            "bloom filter", "cuckoo filter", "xor filter", "hyperloglog", "count-min",
            "t-digest", "skip list", "roaring bitmap", "b-tree", "radix tree", "trie",
            "fenwick", "segment tree", "disjoint set", "union-find", "intrusive heap",
            "lru", "lfu", "crdt", "merkle tree", "myers diff", "suffix array",
            "aho-corasick", "piece table", "gap buffer", "rope", "fuzzy match", "glob",
            "regex", "sorted set", "zset", "hash map", "ring buffer", "minhash", "simhash",
        ],
    ),
    (
        "web",
        "3. Web、HTTP、路由与微服务生态",
        "Web, HTTP, Routing & Microservices",
        {
            "acme", "atom", "cookie", "cors", "css", "data_url", "dom", "finger", "form",
            "gemini", "gopher", "graphql", "grpc", "har", "hpack", "html",
            "html_asset_scanner", "http", "http2", "http3", "jsonrpc", "jwt", "mcp",
            "mime", "multipart", "oauth", "oauth2", "oidc", "openapi", "qpack",
            "rfc7807", "rfc7807_problem", "robots_txt", "router", "rss", "session",
            "sitemap", "sse", "structured_headers", "swagger", "url", "w3c_traceparent",
            "webauthn", "webhook", "websocket", "websocket_frame", "websub", "whois",
            "ws", "xmlrpc", "jsonapi", "hal", "hateoas", "etag", "csp", "hsts",
            "accept", "content_type", "multipart_form", "urlpattern", "fetch", "cgi",
            "fastcgi", "scgi", "uwsgi", "ajp", "icap", "webdav", "caldav", "carddav",
            "activitypub", "webfinger", "atproto", "nostr", "feed", "opml", "microformat",
            "opengraph", "schema_org", "amp", "pwa", "manifest", "service_worker",
            "sri", "uri_normalize", "netrc",
        },
        [
            "http", "websocket", "url", "router", "graphql", "grpc", "json-rpc", "mcp",
            "oauth", "oidc", "jwt", "webauthn", "webhook", "openapi", "robots.txt",
            "sitemap", "rfc 7807", "rfc7807", "structured field", "traceparent", "mime",
            "data url", "html", "css", "rss", "atom", "cookie", "cors", "sse", "hpack",
            "qpack", "fastcgi", "webdav", "whois", "acme", "gemini", "gopher",
        ],
    ),
    (
        "network",
        "4. 网络协议、传输层与分布式通信",
        "Networking, Protocols & Messaging",
        {
            "amqp", "arp", "bencode", "bfd", "bgp", "bittorrent", "cdp", "coap", "dccp",
            "ddns", "dhcp", "dhcpv6", "dht", "diameter", "dkim", "dmarc", "dns", "dnssec",
            "doh", "dot", "dtls", "ebpf", "email", "ftp", "geneve", "gre", "gtp", "hsrp",
            "i2p", "ical", "icmp", "icmpv6", "igmp", "imap", "ipfix", "ipsec", "irc",
            "iscsi", "isis", "kafka", "kerberos", "ldap", "ldif", "lldp", "lwm2m", "mail",
            "matrix", "mdns", "memcached", "mpls", "mqtt", "nats", "net", "netflow",
            "nfs", "nntp", "ntp", "ntpd", "openvpn", "ospf", "pcap", "pcapng", "pop3",
            "ppp", "pppoe", "proxy", "ptp", "quic", "radius", "rdma", "redis", "resp",
            "rip", "rtcp", "rtp", "rtsp", "sctp", "sdp", "sflow", "sftp", "sip", "smb",
            "smtp", "snmp", "socks", "socks5", "spf", "ssh", "stomp", "stp", "stun",
            "syslog", "tacacs", "tailscale", "telnet", "tftp", "tls", "tor", "turn",
            "udp", "tcp", "vcard", "vlan", "vrrp", "vxlan", "wireguard", "xmpp",
            "zeroconf", "ssdp", "upnp", "natpmp", "pcp", "ice", "webrtc", "sctp",
            "rsvp", "pim", "eigrp", "babel", "olsr", "batman", "wire", "ethernet",
            "ipv4", "ipv6", "cidr", "asn_net", "rpki", "mrt", "bmp_bgp", "netlink",
            "openflow", "p4", "pfcp", "ngap", "s1ap", "sccp", "m3ua", "isup", "map",
            "smpp", "icap", "radsec", "eap", "dot1x", "wpa", "wifi", "ieee80211",
            "ieee802154", "6lowpan", "rpl", "zigbee_net", "mqtt5", "pulsar", "zeromq",
            "zmq", "nanomsg", "nng", "capnp_rpc", "tarantool", "beanstalk", "gearman",
            "bpf", "l2tp", "lacp", "llmnr", "nbns",
        },
        [
            "tcp", "udp", "dns", "dhcp", "ntp", "icmp", "bgp", "ospf", "rip", "vxlan",
            "wireguard", "ipsec", "socks", "stun", "turn", "sip", "sdp", "rtp", "rtsp",
            "mqtt", "amqp", "stomp", "nats", "kafka", "redis", "memcached", "smtp",
            "imap", "pop3", "ftp", "tftp", "ssh", "ldap", "radius", "snmp", "syslog",
            "netflow", "ipfix", "pcap", "quic", "sctp", "dtls", "tls", "bittorrent",
            "irc", "xmpp", "coap", "smb", "nfs", "iscsi", "dkim", "dmarc", "spf",
            "vcard", "icalendar", "arp", "lldp", "mpls", "gre", "geneve", "ppp", "vlan",
            "stp", "bfd", "vrrp", "kerberos", "diameter", "gtp", "smpp", "zeromq",
        ],
    ),
    (
        "crypto",
        "5. 密码学、哈希与安全认证",
        "Cryptography, Hashing & Security",
        {
            "abac", "acl", "adler", "aes", "age", "argon", "argon2", "asn1", "audit",
            "bcrypt", "blake", "blake2", "blake3", "bls", "casbin", "chacha", "cityhash",
            "cms_crypto", "cose", "crc", "crc32", "crl", "crypto", "csrf", "cvss",
            "cyclonedx", "der", "dilithium", "ecdh", "ecdsa", "ed25519", "elgamal",
            "farmhash", "fido", "fnv", "gpg", "highway", "hkdf", "hmac", "hotp", "jose",
            "kdf", "keccak", "kyber", "md4", "md5", "metrohash", "minisign", "mlkem",
            "mldsa", "murmur", "ocsp", "otp", "paillier", "passkey", "pbkdf2", "pem",
            "pgp", "pkcs", "poly1305", "pqc", "rbac", "ripemd", "rsa", "salsa", "sbom",
            "scrypt", "secp256k1", "security", "sha", "sha1", "sha2", "sha256", "sha3",
            "shamir", "signify", "sigstore", "siphash", "slsa", "sm2", "sm3", "sm4",
            "spdx", "sphincs", "spiffe", "spooky", "sss", "t1ha", "totp", "tuf",
            "whirlpool", "wyhash", "x25519", "x509", "xxhash", "zkp", "aead", "ascon",
            "camellia", "blowfish", "twofish", "serpent", "des", "rc4", "gmac", "cmac",
            "siv", "ocb", "gcm", "ccm", "noise_proto", "signal_proto", "mls", "hpke",
            "paseto", "macaroon", "biscuit", "branca", "fernet", "nacl", "libsodium",
            "srp", "opaque", "pake", "spake2", "vrf", "vdf", "pedersen", "bulletproof",
            "groth16", "plonk", "stark", "fhe", "bfv", "ckks", "mpc_crypto", "ot",
            "secret_sharing", "threshold", "bip32", "bip39", "bip44", "bech32_crypto",
            "keccak256", "poseidon", "mimc", "pedersen_hash", "crc16", "crc64", "fletcher",
            "luhn", "verhoeff", "damm", "isbn", "ean_check", "policy",
            "cel", "jks", "ntlm", "tpm",
        },
        [
            "cryptograph", "cipher", "aead", "aes", "chacha", "ed25519", "x25519", "rsa",
            "ecdsa", "secp256k1", "bls", "kyber", "dilithium", "post-quantum", "x.509",
            "x509", "asn.1", "pkcs", "ocsp", "pgp", "openpgp", "minisign", "age-encryption",
            "totp", "hotp", "hmac", "hkdf", "pbkdf2", "scrypt", "argon2", "bcrypt",
            "sha-", "sha256", "blake", "md5", "crc", "xxhash", "wyhash", "murmur",
            "siphash", "fnv", "shamir", "zero-knowledge", "paseto", "macaroon", "cose",
            "jose", "hpke", "noise protocol", "rbac", "abac", "sbom", "cvss", "spiffe",
        ],
    ),
    (
        "storage",
        "6. 存储、数据库、文件系统与可观测性",
        "Storage, Databases, Filesystems & Observability",
        {
            "apfs", "arrow", "arrow_ipc", "avro", "badger", "blob", "bolt", "btrfs",
            "car", "cid", "clickhouse", "cpio", "db", "delta", "dmg", "duckdb", "erofs",
            "exfat", "ext2", "ext4", "fat", "fat32", "fits", "gcs", "gguf", "gossip",
            "gpt", "graphite", "hdf5", "hfs", "hudi", "iceberg", "influx", "ipfs",
            "iso9660", "jaeger", "kv", "lance", "leveldb", "lmdb", "loki", "lsm", "lvm",
            "mariadb", "mat", "mbr", "minio", "mssql", "multibase", "multihash", "mvcc",
            "mysql", "netcdf", "npy", "npz", "ntfs", "onnx", "oracle", "orc", "otel",
            "otlp", "parquet", "paxos", "pg", "postgres", "prometheus", "qcow2", "raft",
            "raid", "rocksdb", "rrd", "s3", "safetensors", "sentry", "sql", "sqlite",
            "squashfs", "statsd", "swim", "tds", "tsdb", "udf", "vhd", "vmdk", "vortex",
            "wal", "xfs", "zfs", "zipkin", "cassandra", "cql", "mongodb", "wire_mongo",
            "neo4j", "bolt_neo4j", "cypher", "sparql", "rdf", "turtle", "ntriples",
            "nquads", "jsonld", "level", "sstable", "bbolt", "redb", "sled", "mdb",
            "dbf", "mdb_access", "fdb", "etcd", "consul", "zookeeper", "jute",
            "metrics", "tracing", "span", "opentelemetry", "openmetrics", "promql",
            "logql", "traceql", "victoriametrics", "clickhouse_native", "pgwire",
            "mysql_binlog", "binlog", "cdc", "debezium", "fs_ext", "minixfs", "ufs",
            "f2fs", "jffs2", "ubifs", "romfs", "cramfs", "vxfs", "reiserfs", "bcachefs",
            "hammertime", "vdi", "raw_disk", "luks", "dm_crypt", "verity",
            "analytics", "cef", "migrate", "perf",
        },
        [
            "sqlite", "postgres", "mysql", "sql", "parquet", "arrow", "avro", "orc",
            "hdf5", "netcdf", "fits", "npy", "safetensors", "gguf", "onnx", "ext4",
            "xfs", "btrfs", "zfs", "fat32", "exfat", "ntfs", "apfs", "iso 9660",
            "squashfs", "erofs", "qcow2", "vmdk", "vhd", "gpt", "mbr", "raft", "paxos",
            "gossip", "lsm", "wal", "mvcc", "ipfs", "cid", "multihash", "prometheus",
            "opentelemetry", "otlp", "statsd", "influx", "zipkin", "jaeger", "rrd",
            "blob store", "filesystem", "database", "leveldb", "rocksdb", "lmdb",
        ],
    ),
    (
        "compression",
        "7. 压缩、归档与二进制编解码",
        "Compression, Archiving & Binary Codecs",
        {
            "7z", "ans", "apk", "ar", "arith", "ascii85", "asn", "base32", "base58",
            "base64", "base85", "bech32", "ber", "bincode", "bitpack", "borsh", "braille",
            "brotli", "bson", "bwt", "bzip2", "cab", "capnp", "cbor", "chimp", "deb",
            "deflate", "delta_enc", "encoding", "fast", "flatbuffers", "flexbuffers",
            "fse", "gorilla", "gzip", "hashids", "hex", "huffman", "ion", "ksuid",
            "leb128", "lz4", "lzma", "lzo", "lzw", "morse", "msgpack", "msi", "mtf",
            "nanoid", "objectid", "pack", "per", "percent", "pfor", "postcard", "proto",
            "protobuf", "punycode", "quoted_printable", "rans", "rar", "rlp", "rle",
            "rpm", "sbe", "scale", "serde", "smile", "snappy", "snowflake", "sprintz",
            "sqids", "ssz", "tar", "thrift", "tlv", "ubjson", "ulid", "uuencode", "uuid",
            "varint", "warc", "xdr", "xid", "xz", "yenc", "zip", "zlib", "zstd", "cuid",
            "lz77", "lz78", "lzss", "lzjb", "lzfse", "lzx", "ppmd", "paq", "zpaq",
            "brotli_enc", "zopfli", "pigz", "blosc", "zfp", "sz", "fpzip", "bitstream",
            "golomb", "rice", "elias", "fibonacci_code", "vbyte", "streamvbyte", "zigzag",
            "base16", "base36", "base62", "base91", "z85", "crockford", "proquint",
            "geohash_enc", "typeid", "uuidv7", "flake", "sonyflake", "interchange",
            "hessian", "amf", "amf0", "amf3", "kryo", "pickle", "marshal", "erlang_etf",
            "etf", "bert", "php_serialize", "bplist", "nbt", "anvil", "mca", "region",
            "binary_schema", "flexbuf", "lzh",
        },
        [
            "compress", "decompress", "deflate", "gzip", "zlib", "brotli", "zstd", "lz4",
            "lzma", "bzip2", "snappy", "lzw", "lzo", "rle", "huffman", "arithmetic cod",
            "rans", "bwt", "gorilla", "tar ", "zip ", "7z", "rar", "cab", "warc", "cpio",
            "base64", "base32", "base58", "base85", "bech32", "hex", "uuencode", "yenc",
            "protobuf", "flatbuffers", "cap'n proto", "thrift", "msgpack", "messagepack",
            "cbor", "bson", "ubjson", "xdr", "rlp", "ssz", "borsh", "sbe", "leb128",
            "varint", "ulid", "uuid", "nanoid", "ksuid", "snowflake", "cuid", "sqids",
            "serde", "pickle", "nbt", "etf", "amf", "archive",
        ],
    ),
    (
        "text",
        "8. 文本、文档格式与模板引擎",
        "Text, Document Formats & Templating",
        {
            "ansi", "asciidoc", "bibtex", "bidi", "cldr", "clf", "csv", "csv_stream",
            "cue", "dhall", "djvu", "docx", "dtd", "edn", "ejs", "epub", "fluent", "fmt",
            "frontmatter", "gettext", "grapheme", "grok", "handlebars", "hcl", "human_fmt",
            "i18n", "icu", "ini", "jinja", "jmespath", "json", "json5", "jsonc", "jsonl",
            "jsonpatch", "jsonpath", "jsonpointer", "jsonschema", "jq", "kana", "kdl",
            "l10n", "latex", "liquid", "log", "logfmt", "logger", "man", "markdown",
            "markdown_table", "md", "metaphone", "mustache", "ndjson", "nix", "odf",
            "org", "pdf", "phonetic", "pinyin", "plist", "porter", "ps", "pptx", "pug",
            "readline", "ron", "rst", "rtf", "sexp", "snowball", "soap", "soundex",
            "stemmer", "str", "string", "table", "template", "term", "tex", "text",
            "toml", "translit", "troff", "tsv", "tty", "tui", "unicode", "utf", "xlsx",
            "xml", "xpath", "xsd", "xslt", "yaml", "properties", "envsubst", "printf",
            "sprintf", "strftime", "slug", "inflect", "pluralize", "markdown_it", "gfm",
            "commonmark", "bbcode", "wiki", "mediawiki", "creole", "textile", "pod",
            "texinfo", "chm", "mobi", "azw", "fb2", "cbz", "cbr", "ics", "vcf_card",
            "vtt_text", "subrip", "po", "mo", "xliff", "arb", "resw", "resx", "strings",
            "jsonnet", "starlark", "pkl", "hocon", "cjson", "hjson", "sjson", "geojson_txt",
            "xml_dsig", "rss_text", "opds", "kdl2", "toml2", "csv_parser", "fixed_width",
            "edifact", "x12", "hl7_txt", "marc", "marc21", "ris", "endnote", "csl",
            "charset_label", "gedcom", "mermaid", "odt", "roman", "sheet",
        },
        [
            "json", "yaml", "toml", "ini", "xml", "xpath", "xslt", "plist", "hcl", "cue",
            "kdl", "ron", "edn", "s-expression", "csv", "tsv", "markdown", "frontmatter",
            "restructuredtext", "asciidoc", "org-mode", "latex", "bibtex", "rtf", "docx",
            "xlsx", "pptx", "epub", "pdf", "postscript", "mustache", "handlebars", "jinja",
            "liquid", "template", "unicode", "utf-8", "utf-16", "grapheme", "bidi",
            "stemmer", "soundex", "metaphone", "i18n", "gettext", "fluent", "ansi",
            "terminal", "tui", "human", "logger", "logfmt", "grok", "jsonpath", "jmespath",
            "json schema", "json patch", "json pointer", "jsonl", "ndjson", "string",
        ],
    ),
    (
        "media",
        "9. 图像、音视频与多媒体图形",
        "Media, Audio, Video & 3D Graphics",
        {
            "3ds", "aac", "abc", "aiff", "alac", "alembic", "ass", "au", "audio", "av1",
            "avi", "avif", "aztec", "barcode", "bdf", "bezier", "blurhash", "bmp", "bvh",
            "caf", "code128", "collada", "color", "cur", "dae", "dash", "datamatrix",
            "dds", "dither", "easing", "ebml", "eps", "exif", "exr", "farbfeld", "fbx",
            "flac", "flv", "font", "gif", "glb", "glsl", "gltf", "glyph", "h264", "h265",
            "harfbuzz", "hdr", "heic", "heif", "hevc", "hls", "icc", "ico", "image",
            "image_probe", "iptc", "it", "jpeg", "jpg", "jxl", "ktx", "m3u8", "mesh",
            "midi", "mjpeg", "mkv", "mod", "mp3", "mp4", "mpd", "mpeg", "noise", "obj",
            "ogg", "opus", "otf", "palette", "pbm", "pcf", "pdf417", "perlin", "pgm",
            "ply", "png", "pnm", "ppm", "psd", "qoi", "qrcode", "raytrace", "rtmp", "s3m",
            "sdf", "sf2", "shader", "simplex", "spirv", "spline", "srt", "stl", "sub",
            "svg", "tga", "thumbhash", "tiff", "ttf", "tween", "usd", "video", "vorbis",
            "voxel", "vp8", "vp9", "vrm", "vst", "vtt", "wav", "webm", "webp", "wgsl",
            "woff", "woff2", "worley", "x3d", "xcf", "xm", "xmp", "y4m", "hlsl", "ean",
            "upc", "qr", "dmtx", "apng", "mng", "flif", "pcx", "sgi", "ras", "xbm", "xpm",
            "wbmp", "jp2", "j2k", "raw_img", "dng", "cr2", "nef", "arw", "bayer", "demosaic",
            "yuv", "rgb", "hsv", "oklab", "lab", "cmyk", "srgb", "aces", "lut3d", "cube_lut",
            "icns", "ani", "cur_ico", "spr", "aseprite", "tiled", "tmx", "tsx_map", "ldtk",
            "spine", "lottie", "rive", "swf", "flv_vid", "asf", "wmv", "wma", "rm", "mov",
            "mxf", "nut", "ivf", "smf", "kar", "musicxml", "lilypond", "gpx_tab", "sid",
            "nsf", "spc", "vgm", "psf", "ay", "ym", "sap", "hes", "kss", "gbs", "vgm_chiptune",
            "wavpack", "ape", "tta", "tak", "mpc_audio", "ac3", "eac3", "dts", "amr",
            "speex", "ilbc", "g711", "g722", "g729", "adpcm", "pcm", "dsd", "dsf", "dff",
            "id3", "apev2", "vorbis_comment", "riff", "iff", "aifc", "bwav", "rf64",
            "cuesheet", "m3u", "pls", "xspf", "smi", "ttml", "dfxp", "scc", "cea608",
            "dvbsub", "pgs", "vobsub", "gl", "vulkan", "metal", "dx12", "canvas", "cairo",
            "skia", "raster", "freetype", "truetype", "opentype", "type1", "cff", "svg_path",
            "icosahedron", "marching_cubes", "csg", "bmesh", "halfedge", "subdiv", "catmull",
            "pbr", "brdf", "ibl", "sh_light", "ao", "gi", "pathtrace", "camera", "frustum",
        },
        [
            "image", "png", "jpeg", "gif", "bmp", "webp", "avif", "heif", "jxl", "tiff",
            "tga", "ico", "dds", "ktx", "qoi", "exr", "psd", "xcf", "svg", "exif", "icc",
            "thumbhash", "blurhash", "qr code", "barcode", "color space", "font", "ttf",
            "otf", "woff", "audio", "wav", "flac", "mp3", "aac", "ogg", "opus", "midi",
            "abc music", "tracker", "soundfont", "video", "mp4", "mkv", "ebml", "webm",
            "avi", "flv", "mpeg", "h.264", "h.265", "hevc", "av1", "vp9", "hls", "m3u8",
            "dash", "subtitle", "srt", "vtt", "3d", "obj", "stl", "ply", "gltf", "fbx",
            "collada", "x3d", "usd", "bvh", "voxel", "ray", "shader", "spir-v", "wgsl",
            "glsl", "tween", "easing", "noise", "perlin", "simplex noise", "chiptune",
            "id3", "riff", "cuesheet", "m3u",
        ],
    ),
    (
        "compiler",
        "10. 编译原理、构建系统与开发者工具链",
        "Compilers, Build Systems & DevTools",
        {
            "antlr", "arm64", "assert", "ast", "bash", "bazel", "beam_disasm", "bench",
            "bundler", "clap", "classfile", "cli", "cmake", "coff", "compile_target",
            "compose", "cov", "css_fmt", "dalvik", "dap", "dex", "disasm", "docker",
            "dockerfile", "dwarf", "ebnf", "elf", "flamegraph", "fuzz", "getopt", "git",
            "helm", "hg", "hosted_git_info", "js", "jsx", "jvm", "k8s", "lexer", "license",
            "llvm", "lsp", "luac", "macho", "make", "meson", "mlir", "mock", "ninja",
            "objdump", "oci", "parser", "path", "path_resolve", "pdb", "pe", "peg", "pkg",
            "pprof", "prof", "pyc", "resolver", "riscv", "semver", "sh", "shell",
            "sourcemap", "sql_fmt", "svn", "terraform", "test", "test_runner", "trace",
            "treesitter", "ts", "tsconfig", "tsconfig_paths", "wasm", "wasm_fmt", "wat",
            "which", "x86", "args", "subcommand", "codegen", "ir", "ssa", "regalloc",
            "bytecode", "vm", "jit", "asm", "assembler", "linker", "ld", "rpath", "dyld",
            "so", "dll", "abi", "mangle", "demangle", "itanium", "rust_demangle",
            "stacktrace", "unwind", "eh_frame", "minidump", "coredump", "gdb", "lldb",
            "tap", "junit", "xunit", "lcov", "cobertura", "jacoco", "gcov", "istanbul",
            "sarif", "checkstyle", "eslint", "prettier", "biome", "swc", "oxc", "esbuild",
            "vite", "rollup", "webpack", "npm", "yarn", "pnpm", "cargo", "pip", "wheel",
            "pyproject", "gomod", "gosum", "maven", "gradle", "nuget", "gem", "composer",
            "podspec", "flake_nix", "opam", "cabal", "mix", "rebar", "hex_pm", "pubspec",
            "vcpkg", "conan", "homebrew", "apt", "yum", "pacman", "apk_pkg", "flatpak",
            "snap", "appimage", "pkgbuild", "ebuild", "spec", "gitignore", "editorconfig",
            "gitattributes", "codeowners", "dependabot", "github_actions", "gitlab_ci",
            "jenkins", "circleci", "travis", "justfile", "taskfile", "earthfile", "procfile",
            "systemd", "launchd", "crontab", "supervisord", "nginx", "haproxy", "envoy",
            "caddy", "apache", "traefik", "varnish", "squid", "bind", "zonefile",
            "bundle_options", "char_freq", "idl", "trx",
        },
        [
            "compiler", "bundler", "resolver", "tsconfig", "package.json", "lockfile",
            "sourcemap", "source map", "wasm", "webassembly", "wat", "elf", "pe/coff",
            "mach-o", "dwarf", "pdb", "disassembl", "jvm", "bytecode", "dex", "pyc",
            "luac", "ast", "lexer", "parser", "peg", "ebnf", "lsp", "dap", "git",
            "semver", "docker", "oci", "kubernetes", "helm", "terraform", "cmake",
            "ninja", "bazel", "makefile", "shell", "cli", "argument parser", "test runner",
            "benchmark", "pprof", "flamegraph", "demangle", "sarif", "lcov", "junit",
            "gitignore", "editorconfig", "systemd", "nginx", "zonefile", "path resolve",
            "which", "compile target", "hosted git",
        ],
    ),
    (
        "numeric",
        "11. 数值计算、线性代数与微积分",
        "Numerical Analysis, Linear Algebra & Calculus",
        {
            "akima", "anneal", "autodiff", "bch", "bessel", "beta", "bfgs", "bicgstab",
            "bigint", "bignum", "bisect", "brent", "bspline", "cg", "chebyshev",
            "cholesky", "clifford", "cmaes", "complex", "csc", "csr", "cubic_spline",
            "decimal", "dual", "eigen", "erf", "fdm", "fec", "fem", "fft", "fixed",
            "galois", "gamma", "gauss_quad", "genetic", "gf", "gmres", "hamming",
            "integrate", "interp", "interval", "lbfgs", "ldpc", "legendre", "linalg",
            "lp", "lu", "math", "matrix", "milp", "nelder", "newton", "ntt", "nurbs",
            "octonion", "ode", "optimize", "pchip", "pde", "polar", "poly", "pso", "qp",
            "qr", "quad", "quaternion", "rational", "rbf", "reed_solomon", "rk4",
            "romberg", "root", "simplex_lp", "simpson", "sparse", "special", "svd",
            "turbo", "vec_math", "viterbi", "bdf", "dft", "dct", "dst", "lapack", "blas",
            "tridiag", "toeplitz", "hankel", "vandermonde", "householder", "givens",
            "lanczos", "arnoldi", "power_iter", "jacobi", "gauss_seidel", "sor",
            "multigrid", "precond", "ilu", "ic", "amg", "krylov", "minres", "lsqr",
            "nnls", "bvls", "trust_region", "sqp", "interior_point", "admm", "proximal",
            "fista", "frank_wolfe", "coord_descent", "de", "nsga2", "tabu", "ant_colony",
            "harmony", "bayes_opt", "gp", "kriging", "lagrange", "hermite", "bernstein",
            "bezier_math", "catmull_rom", "loess_math", "padé", "pade", "taylor",
            "cheb_approx", "remez", "minimax", "continued_frac", "euler", "verlet_ode",
            "symplectic", "leapfrog", "adams", "radau", "rosenbrock", "stiff", "dae",
            "shooting", "collocation", "finite_diff", "finite_elem", "finite_vol",
            "spectral_pde", "bem", "mc_int", "vegas", "quasi_mc", "sobol_int", "halton",
            "primes", "sieve_prime", "nt", "number_theory", "modmath", "crt", "pollard",
            "miller_rabin", "lucas", "combinatorics", "perm", "comb", "catalan",
            "stirling", "partition_num", "fibonacci", "bernoulli", "zeta", "lambert_w",
            "elliptic_int", "hypergeom", "airy", "hankel_fn", "spherical_harm",
            "fountain", "raptor", "luby", "erasure", "rs_code", "golay", "convolutional",
            "trellis", "trellis_code", "crc_math", "ieee754", "softfloat", "float16",
            "bfloat16", "posit", "unums", "bigfloat", "mpfr", "arb", "poly_ring",
            "ga",
        },
        [
            "linear algebra", "matrix", "svd", "qr ", "lu ", "cholesky", "eigen",
            "sparse matrix", "conjugate gradient", "gmres", "fft", "dft", "dct", "ntt",
            "polynomial", "chebyshev", "bessel", "gamma function", "ode", "runge-kutta",
            "pde", "finite element", "quadrature", "integration", "newton-raphson",
            "levenberg-marquardt", "root finding", "optimization", "bfgs", "l-bfgs",
            "linear programming", "simplex", "quadratic programming", "simulated annealing",
            "genetic algorithm", "interpolat", "spline", "b-spline", "nurbs", "autodiff",
            "dual number", "interval arithmetic", "big integer", "decimal", "fixed-point",
            "rational", "complex", "quaternion", "galois field", "reed-solomon", "ldpc",
            "viterbi", "hamming", "bch", "erasure", "number theory", "prime", "combinator",
        ],
    ),
    (
        "ml",
        "12. 概率统计、机器学习与 AI Agent",
        "Statistics, Machine Learning & AI Agents",
        {
            "adaboost", "agentmem", "ann", "anova", "arima", "attention", "bandit",
            "bayes", "bm25", "bootstrap", "bpe", "changepoint", "chi2", "cnn", "corr",
            "cov_stat", "crf", "cusum", "dbscan", "dist", "dtree", "ekf", "elasticnet",
            "em", "eval", "faiss", "garch", "gbdt", "ggml", "gibbs", "glm", "gmm", "gru",
            "hdbscan", "hmc", "hmm", "hnsw", "holt", "hypothesis", "ica", "iso_forest",
            "ivf", "kalman", "kmeans", "knn", "ks_test", "lasso", "lda", "linreg", "lof",
            "logreg", "lora", "lstm", "mann_whitney", "markov", "mcmc", "mcts", "mlp",
            "mt19937", "naive_bayes", "nmf", "nn", "ocsvm", "particle", "pca", "pcg",
            "pelt", "pq", "prng", "prob", "prompt", "prophet", "qda", "qlearn", "quant",
            "rag", "rf", "ridge", "rl", "rng", "rnn", "sarima", "sentencepiece", "sobol",
            "spectral", "stat", "stats", "svd_ml", "svm", "tensor", "tfidf", "thompson",
            "tiktoken", "tokenizer", "transformer", "tsne", "ttest", "ucb", "ukf", "umap",
            "unigram", "wilcoxon", "wordpiece", "xgboost", "xoshiro", "random", "rand",
            "splitmix", "chacha_rng", "philox", "threefry", "sfc64", "lehmer", "lcg",
            "kde", "kernel_density", "ecdf", "qq", "shapiro", "anderson", "jarque_bera",
            "durbin_watson", "adf", "kpss", "granger", "coint", "var_ts", "vecm",
            "stl_decomp", "seasonal", "acf", "pacf", "ewma", "bollinger", "survival",
            "kaplan_meier", "cox", "weibull", "hazard", "ab_test", "power_analysis",
            "bayesian", "dirichlet", "beta_binom", "conjugate", "vi", "elbo", "smc",
            "abc_stat", "copula", "mahalanobis", "hotelling", "manova", "cca", "pls",
            "factor_analysis", "mds", "isomap", "lle", "spectral_cluster", "agglomerative",
            "hierarchical", "ward", "birch", "optics", "meanshift", "affinity_prop",
            "som", "kohonen", "rbm", "autoencoder", "vae", "gan", "diffusion", "flow_match",
            "clip", "embedding", "cosine", "dot_prod", "rerank", "chunker", "splitter",
            "agent", "tool_call", "function_call", "memory_agent", "planner", "guardrail",
            "eval_llm", "bleu", "rouge", "meteor", "bertscore", "perplexity", "beam_search",
            "top_k", "top_p", "min_p", "grammar_sample", "logit", "softmax", "activation",
            "loss", "adam", "sgd", "rmsprop", "adagrad", "scheduler_lr", "batchnorm",
            "layernorm", "rmsnorm", "rope_emb", "alibi", "flash_attn", "kv_cache",
            "decision_tree", "random_forest", "gradient_boost", "perceptron", "winnow",
            "online_ml", "ftrl", "pa", "bandits", "contextual_bandit", "linucb", "dqn",
            "ppo", "a2c", "td", "sarsa", "mdp", "pomdp", "value_iter", "policy_iter",
            "bandit_rl", "recommender", "als", "bpr", "fm_ml", "wide_deep", "collaborative",
            "matrix_factor", "rank", "ndcg", "map_ir", "mrr", "roc", "auc", "pr_curve",
            "confusion", "calibration", "platt", "isotonic", "conformal", "shap", "lime",
            "dtw", "huber",
        },
        [
            "logistic regression", "linear regression", "nmf", "non-negative matrix",
            "spectral", "k-means", "kmeans", "dbscan", "gmm", "gaussian mixture", "hmm",
            "hidden markov", "kalman", "particle filter", "holt-winters", "exponential smooth",
            "arima", "time series", "pca", "ica", "t-sne", "umap", "svm", "decision tree",
            "random forest", "gradient boost", "naive bayes", "k-nearest", "knn", "hnsw",
            "ann ", "bm25", "tf-idf", "tokenizer", "bpe", "tiktoken", "tensor", "neural",
            "transformer", "attention", "agent", "rag", "mcmc", "bootstrap", "hypothesis",
            "distribution", "probability", "statistic", "prng", "random number", "bandit",
            "reinforcement", "markov", "changepoint", "survival", "kaplan-meier", "ab test",
            "bleu", "rouge", "isolation forest", "anomaly", "outlier", "conformal",
        ],
    ),
    (
        "signal",
        "13. 信号处理、频谱分析与自动控制",
        "Signal Processing, Spectral Analysis & Control",
        {
            "agc", "am", "ask", "autocorr", "beamform", "bessel_flt", "biquad", "bode",
            "butterworth", "cepstrum", "cheby", "chorus", "cic", "compressor", "costas",
            "cwt", "dsp", "dwt", "elliptic", "esprit", "fir", "flanger", "fm", "fsk",
            "hilbert", "iir", "limiter", "lms", "lomb", "lpc", "mel", "mfcc", "mpc",
            "music_doa", "nlms", "nyquist", "ofdm", "periodogram", "phaser", "pid",
            "pitch", "pll", "pole_zero", "polyphase", "psk", "qam", "resample", "reverb",
            "rls", "root_locus", "savgol", "savitzky", "sdr", "state_space", "stft",
            "wavelet", "welch", "xcorr", "yin", "ztransform", "lqr", "lqg", "hinf",
            "kalman_ctrl", "sliding_mode", "adaptive_ctrl", "fuzzy_ctrl", "servo",
            "stepper", "trajectory", "motion", "s_curve", "trapezoid", "kinematics",
            "ik", "fk", "dh_param", "jacobian_robot", "dynamics", "imu", "madgwick",
            "mahony", "complementary", "ahrs", "sensor_fusion", "odometry", "slam",
            "ekf_slam", "icp", "ndt", "lidar", "radar", "sonar", "cfar", "doppler",
            "chirp", "matched_filter", "wiener", "deconv", "envelope", "peak_detect",
            "zero_cross", "vad", "aec", "ns_audio", "beamformer", "goertzel", "dtmf",
            "window_fn", "hann", "hamming_win", "blackman", "kaiser", "chebwin",
            "filterbank", "qmf", "mdct", "imdct", "pqmf", "resampler", "src", "sinc",
            "farrow", "cordic", "nco", "dds_sig", "mixer", "decimator", "interpolator",
            "equalizer", "eq", "crossover", "linkwitz", "allpass", "comb", "schroeder",
            "fdn", "convolver", "ola", "ols", "vocoder", "phase_vocoder", "psola",
            "wsola", "formant", "autocorrelation", "levinson", "burg", "yule_walker",
        },
        [
            "biquad", "iir", "fir ", "savitzky-golay", "loess", "wavelet", "dwt", "stft",
            "goertzel", "dtmf", "dsp", "signal processing", "filter", "butterworth",
            "chebyshev filter", "mel ", "mfcc", "lpc", "hilbert", "resample", "pid ",
            "lqr", "mpc", "control", "state space", "bode", "nyquist", "pll", "ofdm",
            "qam", "psk", "fsk", "sdr", "imu", "madgwick", "mahony", "ahrs", "kinematics",
            "icp", "slam", "lidar", "radar", "cfar", "wiener", "cordic", "window function",
            "levinson", "burg", "welch", "lomb-scargle", "peak detect", "pitch",
        ],
    ),
    (
        "geo",
        "14. 图论、计算几何与地理空间（GIS）",
        "Graphs, Computational Geometry & Geospatial",
        {
            "aabb", "adsb", "ais", "astar", "astro", "balltree", "bellman", "betweenness",
            "bipartite", "bridges", "bvh", "capsule", "centrality", "clipper", "collision",
            "community", "convex_hull", "dag", "delaunay", "dijkstra", "earcut", "ecef",
            "enu", "epa", "ephemeris", "epsg", "flow", "floyd", "geo", "geodesic",
            "geohash", "geojson", "geom", "geometry", "gjk", "gpx", "graph", "h3",
            "haversine", "hungarian", "jps", "kdtree", "kml", "kruskal", "label_prop",
            "leiden", "louvain", "matching", "mavlink", "maxflow", "mesh2d", "mgrs",
            "mincut", "mvt", "navmesh", "nmea", "obb", "octree", "orbit", "orca",
            "pagerank", "pathfinding", "pbd", "physics", "pluscode", "pmtiles", "poly2d",
            "polygon", "prim", "proj", "quadtree", "rigidbody", "rtree", "rvo", "s2",
            "sat", "scc", "sgp4", "shapefile", "shp", "spatial", "tarjan", "tle", "topo",
            "triangulate", "utm", "verlet", "vincenty", "voronoi", "vptree", "wkb", "wkt",
            "geotiff", "cog", "dem", "srtm", "osm", "pbf_osm", "gtfs", "gpx_geo",
            "polyline", "google_polyline", "geobuf", "topojson", "flatgeobuf", "fgb",
            "las", "laz", "pointcloud", "pcd", "e57", "proj4", "wgs84", "mercator",
            "lambert", "albers", "stereographic", "gnomonic", "great_circle", "rhumb",
            "sun_pos", "moon_pos", "julian", "sidereal", "kepler", "spacetrack", "rinex",
            "rtcm", "ubx", "gps", "gnss", "galileo", "glonass", "beidou", "ais_maritime",
            "aprs", "ax25", "kiss_tnc", "mode_s", "acars", "arinc", "asterix", "metar",
            "taf", "notam", "grib", "bufr", "synop", "shapely", "boolean_poly", "offset_poly",
            "straight_skeleton", "medial_axis", "alpha_shape", "min_enclosing", "welzl",
            "rotating_calipers", "sweep_line", "bentley_ottmann", "dcel", "planar",
            "graph_color", "clique", "independent_set", "vertex_cover", "tsp", "vrp",
            "chinese_postman", "euler_tour", "hamilton", "steiner", "k_shortest", "yen",
            "contraction_hier", "alt_astar", "dstar", "theta_star", "rrt", "prm",
            "clothoid", "dubins", "reeds_shepp", "boids", "flocking", "crowd",
            "verlet_phys", "xpbd", "cloth", "softbody", "fluid", "sph", "mpm", "fem_phys",
            "box2d", "bullet", "chipmunk", "joints", "constraints", "broadphase", "sap",
            "igc", "mst", "segy",
        },
        [
            "graph", "dag", "max flow", "min cut", "hungarian", "bipartite", "mst",
            "dijkstra", "bellman-ford", "floyd-warshall", "a*", "pathfinding", "navmesh",
            "pagerank", "louvain", "community", "tarjan", "k-d tree", "kdtree", "r-tree",
            "quadtree", "octree", "bvh", "spatial", "geometry", "convex hull", "delaunay",
            "voronoi", "earcut", "triangulat", "polygon", "gjk", "collision", "physics",
            "geojson", "wkt", "wkb", "gpx", "kml", "shapefile", "mvt", "pmtiles",
            "geohash", "h3", "s2", "utm", "geodesic", "haversine", "vincenty", "nmea",
            "ais", "ads-b", "mavlink", "tle", "sgp4", "orbit", "ephemeris", "gnss",
            "rinex", "rtcm", "metar", "grib", "osm", "gtfs", "point cloud", "las",
            "dubins", "rrt", "clothoid", "topological", "shortest path", "matching",
        ],
    ),
    (
        "industrial",
        "15. 工控、硬件固件与垂直行业协议",
        "Industrial, Embedded, Firmware & Domain Protocols",
        {
            "ach", "acpi", "artnet", "ata", "bacnet", "bam", "bed", "bic", "biosignal",
            "black_scholes", "ble", "bluetooth", "can", "canopen", "cif", "cusip", "dali",
            "devicetree", "dfu", "dicom", "dmx", "dnp3", "doip", "dtb", "dxf", "ecg",
            "edf", "edid", "emv", "ethercat", "fasta", "fastq", "fhir", "fix", "fix_fin",
            "flexray", "gcode", "gerber", "gff", "hart", "hex_intel", "hid", "hl7", "i2c",
            "iban", "iec104", "iec61850", "iges", "inchi", "isin", "iso20022", "iso8583",
            "j1939", "jtag", "kicad", "knx", "lei", "lin", "lob", "lora", "lorawan",
            "matter", "modbus", "mol", "mt103", "mt940", "nacha", "ndef", "nfc", "nvme",
            "obd", "obd2", "ofx", "ohlcv", "opcua", "orderbook", "osc", "pci", "pcie",
            "pdb_bio", "profibus", "profinet", "qif", "quant_fin", "rfid", "rs485",
            "sacn", "sam", "sata", "scsi", "sdf_chem", "sedol", "sepa", "smbios",
            "smiles", "spi", "spice", "srec", "step", "sv", "swd", "swift", "ta",
            "thread_iot", "tlv_emv", "uart", "uds", "uefi", "uf2", "usb", "vbe", "vcf",
            "verilog", "vhdl", "xbrl", "zigbee", "zwave", "ihex", "motorola_srec",
            "ti_txt", "elf_fw", "fdt", "coreboot", "cbfs", "fit_uimage", "uboot",
            "android_boot", "fastboot", "adb", "mtp", "ptp_usb", "uvc", "uac", "cdc",
            "msc", "dfu_usb", "ble_gatt", "hci", "l2cap", "att", "rfcomm", "obex",
            "sdp_bt", "avdtp", "a2dp", "hfp", "mesh_bt", "ant_plus", "fit_garmin",
            "tcx", "strava", "pwc", "en13757", "mbus", "wmbus", "dlms", "cosem", "ansi_c12",
            "ocpp", "iso15118", "chademo", "openadr", "sunspec", "lonworks", "enOcean",
            "enocean", "zwave_iot", "insteon", "x10", "ir_nec", "rc5", "cec", "ddc",
            "displayport", "hdmi", "mipi", "csi", "dsi", "canfd", "j1708", "j1587",
            "isobus", "nmea2000", "n2k", "mil1553", "arinc429", "spacewire", "ccsds",
            "can_dbc", "dbc", "ldf", "arxml", "a2l", "asap2", "mdf", "mf4", "blf", "asc",
            "trc", "pcap_can", "xcp", "ccp", "kwp2000", "someip", "doip_auto", "avb",
            "tsn", "ptp_1588", "goose", "sv_iec", "mms", "s7", "fins", "mc_proto",
            "eip", "cip", "enip", "devicenet", "controlnet", "sercos", "powerlink",
            "io_link", "asi", "foundation_fieldbus", "cc_link", "mechatrolink", "ethercat_coe",
            "ros", "ros2", "dds_rtps", "rtps", "cdr", "lcm", "mcap", "rosbag", "urdf",
            "sdf_robot", "mjcf", "collada_robot", "cnc", "hpgl", "plt", "excellon",
            "odbpp", "ipc2581", "gdsii", "oasis", "lef", "def", "liberty", "spef", "sdf_eda",
            "vcd", "fst_wave", "ghw", "blif", "aiger", "cnf", "dimacs", "smtlib", "smt",
            "sat_solver", "maxsat", "csp_solver", "ilp", "cp_sat", "dna", "rna", "protein",
            "genbank", "embl", "swissprot", "mmcif", "mol2", "xyz_chem", "gro", "xtc",
            "dcd", "trr", "mzml", "mzxml", "mgf", "fcs", "ome_tiff", "nifti", "nii",
            "minc", "analyze", "bdf_bio", "gdf", "wfdb", "mit_bih", "hl7v2", "cda",
            "ccda", "x12_837", "ncpdp", "snomed", "icd10", "loinc", "rxnorm", "ctakes",
            "itch", "ouch", "soupbintcp", "moldudp", "sbe_cme", "pillar", "utp", "cta",
            "opra", "fpml", "fi_xml", "iso15022", "mt540", "camt", "pacs", "pain",
            "fedwire", "chips", "bacs", "fps", "upi", "pix", "boleto", "qrbill",
            "epc_qr", "emvco", "iso7816", "apdu", "mifare", "desfire", "felica",
            "iso14443", "iso15693", "epc_gen2", "gs1", "gtin", "sscc", "gln", "epcis",
            "rail", "etcs", "ais_ship", "ecdis", "s57", "s100", "iho", "aixm", "iwxxm",
            "chess", "pgn", "fen", "epd", "uci", "sgf", "go_board", "sudoku", "poker",
            "mahjong", "dice", "elo", "glicko", "trueskill", "openskill", "bradley_terry",
            "billing", "fit", "fx", "multiboot",
        },
        [
            "modbus", "can ", "canopen", "j1939", "obd", "uds", "ethercat", "profinet",
            "bacnet", "knx", "opc-ua", "opcua", "iec 61850", "iec 60870", "dnp3", "hart",
            "bluetooth", "ble", "zigbee", "lorawan", "matter", "nfc", "ndef", "rfid",
            "usb", "hid", "pci", "nvme", "scsi", "i2c", "spi", "uart", "dmx", "art-net",
            "osc ", "acpi", "smbios", "uefi", "device tree", "edid", "intel hex", "srec",
            "uf2", "g-code", "gcode", "dxf", "gerber", "kicad", "verilog", "vhdl", "vcd",
            "gdsii", "spice", "dicom", "hl7", "fhir", "edf", "nifti", "fasta", "fastq",
            "sam/bam", "vcf", "pdb", "smiles", "mol", "cif", "fix ", "iso 20022",
            "iso 8583", "swift", "iban", "emv", "apdu", "xbrl", "ofx", "black-scholes",
            "order book", "ohlcv", "itch", "ouch", "ocpp", "dlms", "mbus", "ros", "mcap",
            "urdf", "dbc", "mdf", "ccsds", "arinc", "dimacs", "smt", "chess", "pgn",
            "fen", "sgf", "elo", "glicko", "trueskill", "garmin fit", "firmware",
        ],
    ),
]


def detect_origin(mod_path: Path) -> str:
    """Return 'Bun' if any .cp file in module has a real Bun source header, else 'Pure-CP'."""
    cp_files = [mod_path] if mod_path.is_file() else sorted(mod_path.rglob("*.cp"))
    has_bun = False
    for f in cp_files:
        try:
            first = f.read_text(encoding="utf-8", errors="ignore").splitlines()[0]
        except IndexError:
            continue
        if BUN_SRC_RE.search(first):
            has_bun = True
            break
    return "Bun" if has_bun else "Pure-CP"


def extract_summary(mod_name: str, mod_path: Path) -> str:
    """Extract concise 1-line summary from doc/lib/<mod>.md or module header comment."""
    doc_file = DOC_ROOT / f"{mod_name}.md"
    if doc_file.is_file():
        lines = doc_file.read_text(encoding="utf-8", errors="ignore").splitlines()
        in_code = False
        for line in lines:
            s = line.strip()
            if s.startswith("```"):
                in_code = not in_code
                continue
            if in_code or not s:
                continue
            if s.startswith("//") or s.startswith("import "):
                continue
            if s.startswith("#"):
                # e.g. `# cap:logreg — 纯 CP 零分配逻辑回归库`
                cleaned = re.sub(r"^#+\s*", "", s)
                for sep in (" — ", " - ", " —", " – ", ": "):
                    if sep in cleaned:
                        right = cleaned.split(sep, 1)[1].strip()
                        if right and not right.lower().startswith("cap:"):
                            return right[:110]
                continue
            if s.startswith(">") or s.startswith("|") or s.startswith("---"):
                continue
            cleaned = re.sub(r"\*\*([^*]+)\*\*", r"\1", s)
            cleaned = re.sub(r"`([^`]+)`", r"\1", cleaned)
            return cleaned[:110]

    # Fallback: read parenthetical description from module's primary .cp header
    cp_files = [mod_path] if mod_path.is_file() else sorted(mod_path.glob("*.cp"))
    for f in cp_files:
        if f.name == "index.cp" and len(cp_files) > 1:
            continue
        try:
            first = f.read_text(encoding="utf-8", errors="ignore").splitlines()[0]
        except IndexError:
            continue
        m = PAREN_DESC_RE.search(first)
        if m:
            return m.group(1).strip()[:110]
    return f"cap:{mod_name} 标准库模块"


def classify_module(mod_name: str, summary: str) -> str:
    """Return category_id for a module based on exact name, token overlap, and summary keywords."""
    lname = mod_name.lower()
    lsum = summary.lower()
    name_parts = set(re.split(r"[_-]+", lname))

    # Pass 1: exact module name match
    for cat_id, _, _, tokens, _ in CATEGORIES:
        if lname in tokens:
            return cat_id

    # Pass 2: token & prefix match on module name + description keyword scoring
    best_cat = ""
    best_score = 0
    for cat_id, _, _, tokens, kws in CATEGORIES:
        score = 0
        for part in name_parts:
            if len(part) >= 3 and part in tokens:
                score += 6
        for tok in tokens:
            if len(tok) >= 4 and tok in lname:
                score += 4
        for kw in kws:
            if kw in lsum:
                score += 3
        if score > best_score:
            best_score = score
            best_cat = cat_id

    if best_score > 0:
        return best_cat
    return "misc"


def discover_modules() -> list[tuple[str, Path]]:
    """Discover all top-level stdlib modules (directories + top-level .cp files excluding index.cp)."""
    mods: dict[str, Path] = {}
    for entry in sorted(LIB_ROOT.iterdir()):
        if entry.name.startswith("."):
            continue
        if entry.is_dir():
            mods[entry.name] = entry
        elif entry.is_file() and entry.suffix == ".cp" and entry.name != "index.cp":
            if entry.stem not in mods:
                mods[entry.stem] = entry
    return sorted(mods.items(), key=lambda kv: kv[0])


def main() -> None:
    modules = discover_modules()
    grouped: dict[str, list[tuple[str, str, str, bool]]] = defaultdict(list)

    total_bun = 0
    total_pure = 0
    seen: set[str] = set()

    for mod_name, mod_path in modules:
        assert mod_name not in seen, f"Duplicate module: {mod_name}"
        seen.add(mod_name)
        origin = detect_origin(mod_path)
        if origin == "Bun":
            total_bun += 1
        else:
            total_pure += 1
        summary = extract_summary(mod_name, mod_path)
        cat_id = classify_module(mod_name, summary)
        has_doc = (DOC_ROOT / f"{mod_name}.md").is_file()
        grouped[cat_id].append((mod_name, origin, summary, has_doc))

    cat_meta = {cid: (zh, en) for cid, zh, en, _, _ in CATEGORIES}
    cat_meta["misc"] = ("16. 其他通用工具与扩展模块", "Other Utilities & Extensions")
    cat_order = [cid for cid, _, _, _, _ in CATEGORIES]
    if grouped["misc"]:
        cat_order.append("misc")

    # Verify 100% coverage without loss or duplication
    classified_total = sum(len(grouped[c]) for c in cat_order)
    assert classified_total == len(modules), f"Mismatch: {classified_total} != {len(modules)}"

    lines: list[str] = []
    lines.append("# CP 标准库按领域分类总览目录 (`cap:*` Catalog)")
    lines.append("")
    lines.append("> 本文档由 `scripts/gen_lib_catalog.py` 自动扫描 `lib/` 全部模块及溯源头生成，100% 覆盖无遗漏、无重复。")
    lines.append("> 标签说明：`[Bun]` = 吸收自 `/data/dev/bun/src/` 的 Bun 衍生库；`[Pure-CP]` = 100% 纯 CP 原创零依赖标准库。")
    lines.append("")
    lines.append("## 总体统计与领域分布")
    lines.append("")
    lines.append(f"- **标准库模块总数**：**{len(modules)}** 个（`[Bun]` **{total_bun}** 个 / `[Pure-CP]` **{total_pure}** 个）")
    lines.append("")
    lines.append("| 领域分类 | 英文域 | 模块数 | `[Bun]` | `[Pure-CP]` |")
    lines.append("| :--- | :--- | :---: | :---: | :---: |")

    for cid in cat_order:
        items = grouped[cid]
        if not items:
            continue
        zh, en = cat_meta[cid]
        b_cnt = sum(1 for _, o, _, _ in items if o == "Bun")
        p_cnt = len(items) - b_cnt
        anchor = re.sub(r"[^\w\u4e00-\u9fff -]", "", zh).strip().replace(" ", "-").lower()
        lines.append(f"| [{zh}](#{anchor}) | {en} | {len(items)} | {b_cnt} | {p_cnt} |")

    lines.append("")
    lines.append("---")
    lines.append("")

    for cid in cat_order:
        items = sorted(grouped[cid], key=lambda x: x[0])
        if not items:
            continue
        zh, en = cat_meta[cid]
        b_cnt = sum(1 for _, o, _, _ in items if o == "Bun")
        p_cnt = len(items) - b_cnt
        lines.append(f"## {zh}")
        lines.append("")
        lines.append(f"*{en}* — 共 **{len(items)}** 个模块（`[Bun]`: {b_cnt}，`[Pure-CP]`: {p_cnt}）")
        lines.append("")
        lines.append("| 模块 | 来源 | 职责摘要 |")
        lines.append("| :--- | :---: | :--- |")
        for mod_name, origin, summary, has_doc in items:
            link = f"[`cap:{mod_name}`](./{mod_name}.md)" if has_doc else f"`cap:{mod_name}`"
            safe_sum = summary.replace("|", "\\|")
            lines.append(f"| {link} | `[{origin}]` | {safe_sum} |")
        lines.append("")

    OUT_PATH.write_text("\n".join(lines), encoding="utf-8")

    print(f"[CATALOG] total={len(modules)} (Bun={total_bun}, Pure-CP={total_pure}) -> {OUT_PATH}")
    for cid in cat_order:
        items = grouped[cid]
        if not items:
            continue
        zh, _ = cat_meta[cid]
        b_cnt = sum(1 for _, o, _, _ in items if o == "Bun")
        p_cnt = len(items) - b_cnt
        print(f"  {zh}: {len(items)} (Bun={b_cnt}, Pure-CP={p_cnt})")
    if grouped["misc"]:
        print("  [MISC SAMPLE]:", [m for m, _, _, _ in grouped["misc"][:25]])


if __name__ == "__main__":
    main()
