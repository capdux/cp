// Migrated from TypeScript (assets/worker/worker-http-import.js) via cap from -ts

@print("hello from worker-http-import.js");
importScripts("./import-me.js");
@print("successfully imported");
self.postMessage("finished");
