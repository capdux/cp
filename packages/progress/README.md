[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# progress

Zero-heap terminal progress bar renderer in pure CP supporting `:bar`, `:percent`, `:current`, `:total`, `:etas`, and `:elapseds` tokens.

## API

- `newProgressBar(total: u64, width: usize): ProgressBar`
- `bar.tick(delta)` / `bar.ratio()` / `bar.percent()` / `bar.isComplete()` / `bar.etaSeconds()`
- `renderProgress(bar: ProgressBar, template: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`progress@2.0.3`)
- **Upstream Repository**: https://github.com/visionmedia/node-progress
- **License**: MIT (clean-room CP implementation)
