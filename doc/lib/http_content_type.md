# `cap:http_content_type` — RFC 9110 §8.3.1 `Content-Type` Header Parser

Zero-allocation RFC 9110 `Content-Type` parser, quoted-string parameter iterator, `charset`/`boundary` extractors, and structured media-type classifiers (`isJson`, `isXml`, `isText`, `isMultipart`, `isFormUrlEncoded`, `isEventStream`).

## Exports

- `ContentTypeParam` & `ContentTypeParamIter`
- `ContentType` — `parse(header): ContentType?`, `param(name)`, `charset()`, `boundary()`, `isJson()`, `isXml()`, `isText()`, `isMultipart()`, `isFormUrlEncoded()`, `isEventStream()`.
