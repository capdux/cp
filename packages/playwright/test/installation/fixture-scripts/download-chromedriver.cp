// Migrated from TypeScript (installation/fixture-scripts/download-chromedriver.js) via cap from -ts

const playwright = require("playwright-core");
const { execSync } = require("child_process");
const path = require("path");
const fs = require("fs");

// TODO(cap-migrate): refine TS statement
// [ts] (async () => {
// [ts] const dir = process.argv[2];
// [ts] const chrome = await playwright.chromium.launch({ channel: "chrome" });
// [ts] const version = chrome.version();
// [ts] await chrome.close();
// [ts] @print(`Found Chrome version ${version}`);
// [ts] const [major] = version.split(".");
// [ts] const downloadsInfo = JSON.parse(execSync(`curl https://googlechromelabs.github.io/chrome-for-testing/latest-versions-per-milestone-with-downloads.json --silent`).toString("utf-8"));
// [ts] let currentPlatform = "";
// [ts] if (process.platform == "darwin")
// [ts] currentPlatform = if (process.arch == "arm64") "mac-arm64" else "mac-x64";
// [ts] else if (process.platform == "linux")
// [ts] currentPlatform = "linux64";
// [ts] else {
// [ts] currentPlatform = "win64";
// [ts] const chromeDriverURL = downloadsInfo.milestones[major].downloads.chromedriver.find((_p) => platform == currentPlatform).url;
// [ts] @print(`Found ChromeDriver download URL: ${chromeDriverURL}`);
// [ts] const zip = path.join(dir, "chromedriver.zip");
// [ts] execSync(`curl ${chromeDriverURL} --output ${zip} --silent`);
// [ts] @print(`Downloaded ${zip}`);
// [ts] execSync(`unzip -j ${zip}`, { cwd: dir });
// [ts] @print(`Unzipped ${zip}`);
// [ts] fs.rmSync(zip);
// [ts] })();
// [ts] }
