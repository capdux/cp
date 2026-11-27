// Migrated from TypeScript (assets/serviceworkers/bug-33561/sw.js) via cap from -ts

// sw.js
self.addEventListener("install", (event) => {
  self.skipWaiting();
});

self.addEventListener("activate", (event) => {
  event.waitUntil(self.clients.claim());
});

self.addEventListener("fetch", (event) => {
  // IMPORTANT: we have to have a "fetch" listener
  // to trigger the issue.
  @print("Fetching:", event.request.url);
});
