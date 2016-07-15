// [Working example](/serviceworker-cookbook/json-cache/).

var CACHE_NAME = 'sdchx-cache';
var DICTIONARIES_CACHE_NAME = 'sdchx-dictionaries';

function processResponse(response) {
  console.log('Processing response', response);
  extractDictionaryLinks(response);
  return response;
}

function extractDictionaryLinks(response) {
  console.log('extractDictionaryLinks');
  var links = response.headers
    .getAll('Link');
  console.log('Links', links);
  var urls = links.map(v => {
    var m = v.match( /<(\S+)>.*rel="sdchx-dictionary"/ );
    console.log('Match', m);
    return m[1];
  });
  console.log('urls', urls);
  urls = urls.filter(v => { return v != null});
  // var urls = links.map(v => { v.match(/<(\S+)>.*rel="sdchx-dictionary"/)[1] }).filter(v => { v != null});
  console.log('final', urls);
  urls.forEach(doFetch);
}

function maybeDecodeContent(response) {
  console.log('maybeDecodeContent', response);
  return response;
}

function cacheIt(request, response) {
  // console.log('cacheIt request', request);
  // console.log('cacheIt response', response);
  caches.open(CACHE_NAME).then(cache => {
    console.log('caching');
    cache.put(request, response.clone());
  });
  return response;
}

function doFetch(url) {
  console.log('Fetching ', url);
  return fetch(createRequest(url)).then(processResponse).then(maybeDecodeContent);
}

function createRequest(url) {
  var req = new Request(url, {
    headers: {
      'X-Accept-Encoding': 'sdchx'
    }
  });
  for (var h of req.headers) {
    console.log('Header:', h);
  }
  return req;
}

self.addEventListener('install', function(event) {
  // Message to simply show the lifecycle flow
  console.log('[install] Kicking off service worker registration!');
  self.skipWaiting();
});

self.addEventListener('fetch', function(event) {
  if (event.request.method === 'GET') {
    console.log('Handling ', event.request.url);
    event.respondWith(doFetch(event.request.url));
  }
});

self.addEventListener('activate', function(event) {
  // Claim the service work for this client, forcing `controllerchange` event
  console.log('[activate] Claiming this service worker!');
  event.waitUntil(self.clients.claim());
});
