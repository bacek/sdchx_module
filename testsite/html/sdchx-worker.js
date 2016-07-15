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
    return m[1];
  });
  urls = urls.filter(v => {
    return v !== null;
  });
  // let urls = links.map(v => { v.match(/<(\S+)>.*rel="sdchx-dictionary"/)[1] }).filter(v => { v != null});
  console.log("final", urls);
  urls.forEach(link => {
    doFetch(new Request(link));
  });
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
  caches.open(CACHE_NAME).then(cache => {
    console.log("caching", request.url);
    cache.put(request, response.clone());
  });
  return response;
}

/**
 * Try to fetch resource from cache. Fallback to network fetch in case of error
 */
function fetchFromCache(request) {
  return caches.open(CACHE_NAME).then(cache => {
    return cache.match(request).then(response => {
      if (response) {
        console.log('Return cached', request.url);
        return response;
      }

      console.log('Do network fetch');
      return fetch(request).then(resp => {
        return cacheIt(request, resp);
      });
    });
  });
}

/**
 * Actually fetch url.
 */
function doFetch(request) {
  console.log("Fetching ", request.url);
  return fetchFromCache(createRequest(request))
      .then(processResponse) // processResponse should after cacheIt() inside
                             // fetchFromCache. But for
                             // now it simplify development to be able to
                             // process cached results
      .then(maybeDecodeContent);
}

/**
 * Create SDCHx-enabled GET request for given url
 */
function createRequest(original) {
  let options = {
    headers: {
      "X-Accept-Encoding": "sdchx"
    }
  };
  for (let h of original.headers) {
    options.headers[h[0]] = h[1];
  }

  return new Request(original.url, options);
}

self.addEventListener("install", event => {
  // Message to simply show the lifecycle flow
  console.log("[install] Kicking off service worker registration!");
  self.skipWaiting();
});

self.addEventListener("fetch", event => {
  if (event.request.method === "GET") {
    console.log("Handling ", event.request.url);
    event.respondWith(doFetch(event.request));
  }
});

self.addEventListener("activate", event => {
  // Claim the service work for this client, forcing `controllerchange` event
  console.log("[activate] Claiming this service worker!");
  event.waitUntil(self.clients.claim());
});
