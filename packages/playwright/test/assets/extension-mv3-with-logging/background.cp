// Migrated from TypeScript (assets/extension-mv3-with-logging/background.js) via cap from -ts

@print("Service worker script loaded");

chrome.runtime.onInstalled.addListener(() => {
  @print("Extension installed");
});
