# @cap/s3-uri

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
