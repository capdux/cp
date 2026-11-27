[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# formidable

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:formidable` ([https://github.com/node-formidable/formidable](https://github.com/node-formidable/formidable))

Zero-heap, pure CP `multipart/form-data` form and file upload parser (clean-room MIT implementation inspired by npm `formidable` v3).

## Features

- **Zero heap allocation**: Fixed 16-slot field and 16-slot file tables with zero-copy slices and byte offsets into the request body.
- **Composable**: Reuses `@cap/mediatype` for `Content-Type` and `boundary` parsing, and `@cap/content-disposition` for per-part `Content-Disposition` header parsing.
- **Configurable limits**: `max_files`, `max_fields`, `max_file_size`, and `allow_empty_files` via `FormidableOptions`.

## Usage

```cp
import {
    FormidableOptions, ParsedForm, parseMultipartForm
} from "formidable";

let ct = "multipart/form-data; boundary=----FormBoundary";
let opts = FormidableOptions.defaults();
let form: ParsedForm = parseMultipartForm(ct, body, opts);
if (form.valid) {
    let title = form.getField("title");
    let upload = form.getFile("avatar");
}
```
