[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/s3-uri

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:amazon-s3-uri` ([https://github.com/jvdl/amazon-s3-uri](https://github.com/jvdl/amazon-s3-uri)) / `crates.io:aws-sdk-s3` ([https://github.com/awslabs/aws-sdk-rust](https://github.com/awslabs/aws-sdk-rust))（原库名 `amazon-s3-uri` → CP 包名 `@cap/s3-uri`）

Zero-heap AWS S3 / R2 / MinIO URI parser (`s3://`, virtual-hosted-style, path-style) and bucket name validator for CP.
Compatible with npm `amazon-s3-uri`.

## API

- Styles: `STYLE_S3`, `STYLE_VIRTUAL_HOSTED`, `STYLE_PATH`
- `S3Uri { style, bucket, key, region, valid }`
- `isValidBucketName(name: string): bool`
- `parseS3Uri(input: string): S3Uri`
- `formatS3Uri(bucket: string, key: string, out: []u8): usize`

## License

MIT
