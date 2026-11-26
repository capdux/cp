<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`curl`](https://github.com/curl/curl)（`curl`）（包名 `@cap/curl`）。

# @cap/curl

Official CP binding for **libcurl (v8.23.0)** — multiprotocol network transfer library (HTTP, FTP, Proxy, Multipart) shipped with a prebuilt stripped `libcurl.so` and a thin C shim (`c/curl_ffi.c`) for zero-wait builds.

## Usage

```cp
import curl from "@cap/curl";
import { println } from "cap:io";

pub fn main(): void! {
    curl.globalInit()?;
    defer curl.globalCleanup();

    println(curl.version());

    let res = curl.httpGet("http://127.0.0.1:8080/ping")?;
    println(res.body());
}
```

## License

curl License (MIT-like). See [LICENSE](./LICENSE).
