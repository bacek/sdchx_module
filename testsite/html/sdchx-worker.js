const CACHE_NAME = "sdchx-cache";
// const DICTIONARIES_CACHE_NAME = "sdchx-dictionaries";

/**
 * Process response from network
 */
function processResponse(response) {
  console.log("Processing response", response);
  extractDictionaryLinks(response);
  return response;
}
/**
 * Extract Link headers from response and fetch advertised dictionaries
 */
function extractDictionaryLinks(response) {
  console.log("extractDictionaryLinks");
  let links = response.headers
    .getAll("Link");
  console.log("Links", links);
  let urls = links.map(v => {
    let m = v.match(/<(\S+)>.*rel="sdchx-dictionary"/);
    console.log("Match", m);
    return m[1];
  });
  console.log("urls", urls);
  urls = urls.filter(v => {
    return v !== null;
  });
  // let urls = links.map(v => { v.match(/<(\S+)>.*rel="sdchx-dictionary"/)[1] }).filter(v => { v != null});
  console.log("final", urls);
  urls.forEach(doFetch);
}
/**
 * Decode VCDiff encoded content.
 * Browser will unzip it for us.
 */
function maybeDecodeContent(response) {
  console.log("maybeDecodeContent", response);
  return response;
}
/**
 * Cache result from network request
 */
function cacheIt(request, response) {
  // console.log("cacheIt request", request);
  // console.log("cacheIt response", response);
  caches.open(CACHE_NAME).then(cache => {
    console.log("caching");
    cache.put(request, response.clone());
  });
  return response;
}
/**
 * Actually fetch url.
 */
function doFetch(url) {
  console.log("Fetching ", url);
  return fetch(createRequest(url))
          .then(cacheIt)
          .then(processResponse)
          .then(maybeDecodeContent);
}

/**
 * Create SDCHx-enabled GET request for given url
 */
function createRequest(url) {
  let req = new Request(url, {
    headers: {
      "X-Accept-Encoding": "sdchx"
    }
  });
  for (let h of req.headers) {
    console.log("Header:", h);
  }
  return req;
}

self.addEventListener("install", event => {
  // Message to simply show the lifecycle flow
  console.log("[install] Kicking off service worker registration!");
  self.skipWaiting();
});

self.addEventListener("fetch", event => {
  if (event.request.method === "GET") {
    console.log("Handling ", event.request.url);
    event.respondWith(doFetch(event.request.url));
  }
});

self.addEventListener("activate", event => {
  // Claim the service work for this client, forcing `controllerchange` event
  console.log("[activate] Claiming this service worker!");
  event.waitUntil(self.clients.claim());
});
