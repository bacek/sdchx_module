#include <assert.h>

#include "sdchx_module.h"

#include "sdchx_config.h"
#include "sdchx_dictionary.h"
#include "sdchx_pool_alloc.h"
#include "sdchx_request_context.h"
#include "sdchx_main_config.h"

namespace sdchx {

std::string to_string(const ngx_str_t& str) {
  return std::string(reinterpret_cast<char*>(str.data), str.len);
}

static ngx_int_t filter_init(ngx_conf_t* cf);
static void* create_conf(ngx_conf_t* cf);
static char* merge_conf(ngx_conf_t* cf, void* parent, void* child);
static void* create_main_conf(ngx_conf_t* cf);
static char* init_main_conf(ngx_conf_t* cf, void* conf);

static char* sdchx_dictionary_block(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static ngx_conf_bitmask_t  ngx_http_sdchx_proxied_mask[] = {
    { ngx_string("off"), NGX_HTTP_GZIP_PROXIED_OFF },
    { ngx_string("expired"), NGX_HTTP_GZIP_PROXIED_EXPIRED },
    { ngx_string("no-cache"), NGX_HTTP_GZIP_PROXIED_NO_CACHE },
    { ngx_string("no-store"), NGX_HTTP_GZIP_PROXIED_NO_STORE },
    { ngx_string("private"), NGX_HTTP_GZIP_PROXIED_PRIVATE },
    { ngx_string("no_last_modified"), NGX_HTTP_GZIP_PROXIED_NO_LM },
    { ngx_string("no_etag"), NGX_HTTP_GZIP_PROXIED_NO_ETAG },
    { ngx_string("auth"), NGX_HTTP_GZIP_PROXIED_AUTH },
    { ngx_string("any"), NGX_HTTP_GZIP_PROXIED_ANY },
    { ngx_null_string, 0 }
};
static ngx_str_t  ngx_http_gzip_no_cache = ngx_string("no-cache");
static ngx_str_t  ngx_http_gzip_no_store = ngx_string("no-store");
static ngx_str_t  ngx_http_gzip_private = ngx_string("private");

static ngx_conf_num_bounds_t stor_size_bounds = {
    ngx_conf_check_num_bounds, 1, 0xffffffffU
};

static ngx_str_t sdchx_default_types[] = {
    ngx_string("text/html"),
    ngx_string("text/css"),
    ngx_string("application/javascript"),
    ngx_string("application/x-sdch-dictionary"),
    ngx_null_string
};

static ngx_str_t nodict_default_types[] = {
    ngx_string("application/x-sdch-dictionary"),
    ngx_null_string
};

static ngx_command_t  filter_commands[] = {

    { ngx_string("sdchx"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, enable),
      NULL },

    { ngx_string("sdchx_dictionary"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_TAKE1|NGX_CONF_BLOCK,
      sdchx_dictionary_block,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  sdchx_module_ctx = {
    NULL,                    /* preconfiguration */
    filter_init,             /* postconfiguration */

    create_main_conf,        /* create main configuration */
    init_main_conf,          /* init main configuration */

    NULL,                 /* create server configuration */
    NULL,                 /* merge server configuration */

    create_conf,             /* create location configuration */
    merge_conf               /* merge location configuration */
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

#if 0
#include <execinfo.h>
static void
backtrace_log(ngx_log_t *log)
{
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    unsigned i;

    for (i = 0; i < frames; ++i) {
        ngx_log_error(NGX_LOG_INFO, log, 0, "frame %d: %s", i, strs[i]);
    }
}
#endif

static ngx_table_elt_t* header_find(ngx_list_t* headers,
                                    const char* key,
                                    ngx_str_t* value) {
  size_t keylen = strlen(key);
  ngx_list_part_t* part = &headers->part;
  ngx_table_elt_t* data = static_cast<ngx_table_elt_t*>(part->elts);
  unsigned i;

  for (i = 0;; i++) {

    if (i >= part->nelts) {
      if (part->next == NULL) {
        break;
      }

      part = part->next;
      data = static_cast<ngx_table_elt_t*>(part->elts);
      i = 0;
    }
    if (data[i].key.len == keylen &&
        ngx_strncasecmp(data[i].key.data, (u_char*)key, keylen) == 0) {
      if (value) {
        *value = data[i].value;
      }
      return &data[i];
    }
  }
  return 0;
}

template <size_t K, size_t V>
ngx_int_t create_output_header(ngx_http_request_t* r,
                               const char (&key)[K],
                               const char (&value)[V],
                               ngx_table_elt_t* prev = NULL) {
  ngx_table_elt_t* h = prev
                       ? prev
                       : static_cast<ngx_table_elt_t*>(
                            ngx_list_push(&r->headers_out.headers));
  if (h == NULL) {
    return NGX_ERROR;
  }

  h->hash = 1;
  ngx_str_set(&h->key, key);
  ngx_str_set(&h->value, value);

  return NGX_OK;
}

// Check should we process request at all
bool should_process(ngx_http_request_t* r, Config* conf) {
  if (!conf->enable) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: not enabled");

    return false;
  }

  if (r->headers_out.status != NGX_HTTP_OK) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported status");

    return false;
  }

  if (r->headers_out.content_encoding
    && r->headers_out.content_encoding->value.len) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is already encoded");
    return false;
  }

#if 0
  if (r->headers_out.content_length_n != -1
    && r->headers_out.content_length_n < conf->min_length) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: content is too small");
    return false;
  }

  if (ngx_http_test_content_type(r, &conf->types) == NULL) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdch header: unsupported content type");
    return false;
  }
#endif

  return true;
}

static ngx_int_t
header_filter(ngx_http_request_t *r)
{
  ngx_log_debug(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdch filter header 000");

  Config* conf = Config::get(r);

  ngx_str_t val;
  if (header_find(&r->headers_in.headers, "accept-encoding", &val) == 0 ||
      ngx_strstrn(val.data, const_cast<char*>("sdchx"), val.len) == 0) { // XXX
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "sdchx header: no 'sdchx' in accept-encoding");
    return ngx_http_next_header_filter(r);
  }

  if (!should_process(r, conf)) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  0,
                  "sdchx header: skipping request");
    return ngx_http_next_header_filter(r);
  }

  if (r->header_only) {
    return ngx_http_next_header_filter(r);
  }

  return ngx_http_next_header_filter(r);
}


ngx_int_t
body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
  RequestContext* ctx = RequestContext::get(r);

  ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "sdchx body");

  if (ctx == NULL || ctx->done || r->header_only) {
    return ngx_http_next_body_filter(r, in);
  }

  return ngx_http_next_body_filter(r, in);

#if 0
  if (!ctx->started) {
    ctx->started = true;
    for (Handler* h = ctx->handler; h; h = h->next()) {
      if (!h->init(ctx)) {
        ctx->done = true;
        return NGX_ERROR;
      }
    }
  }

  ngx_log_debug0(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "sdchx filter started");

  // cycle while there is data to handle
  for (; in; in = in->next) {
    if (in->buf->flush) {
      ctx->need_flush = true;
    }

    off_t buf_size = ngx_buf_size(in->buf);
    Status status = ctx->handler->on_data(in->buf->pos, buf_size);
    in->buf->pos = in->buf->last;
    ctx->total_in += buf_size;

    if (status == STATUS_ERROR) {
      ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "sdch failed");
      ctx->done = true;
      return NGX_ERROR;
    }

    if (in->buf->last_buf) {
      ngx_log_debug(NGX_LOG_DEBUG_HTTP,
          ctx->request->connection->log, 0, "closing ctx");
      ctx->done = true;
      return ctx->handler->on_finish() == STATUS_OK ? NGX_OK : NGX_ERROR;
    }
  }
#endif

  return NGX_OK;
}

void *
create_main_conf(ngx_conf_t *cf)
{
    return POOL_ALLOC(cf, MainConfig);
}

char *
init_main_conf(ngx_conf_t *cf, void *cnf)
{
    MainConfig *conf = static_cast<MainConfig*>(cnf);
    /* 
    if (conf->stor_size != NGX_CONF_UNSET_SIZE)
        conf->fastdict_factory.set_max_size(conf->stor_size);
    */
    return NGX_CONF_OK;
}

void *
create_conf(ngx_conf_t *cf)
{
  return POOL_ALLOC(cf, Config, cf->pool);
}


char*
sdchx_dictionary_param(ngx_conf_t* cf, ngx_command_t* dummy, void* cnf) {
  Dictionary* dict = reinterpret_cast<Dictionary*>(cf->ctx);

  if (cf->args->nelts != 2) {
    return const_cast<char*>("Incorrect command");
  }

  ngx_str_t* value = reinterpret_cast<ngx_str_t*>(cf->args->elts);

  ngx_log_error(NGX_LOG_INFO, cf->log, 0, "Handling %*s -> %*s",
    value[0].len, value[0].data, value[1].len, value[1].data);

  if (ngx_strcmp(value[0].data, "url") == 0) {
    dict->set_url(to_string(value[1]));
  } else if (ngx_strcmp(value[0].data, "file") == 0) {
    dict->set_filename(to_string(value[1]));
  } else if (ngx_strcmp(value[0].data, "tag") == 0) {
    dict->set_tag(to_string(value[1]));
  } else if (ngx_strcmp(value[0].data, "algo") == 0) {
    dict->set_algo(to_string(value[1]));
  } else if (ngx_strcmp(value[0].data, "max-age") == 0) {
    char* ptr = reinterpret_cast<char*>(value[1].data);
    dict->set_max_age(strtol(ptr, NULL, 10));  // TODO(bacek): handle errors
  } else {
    return const_cast<char*>("Unknown parameter");
  }


  return NGX_CONF_OK;
}

char *
sdchx_dictionary_block(ngx_conf_t *cf, ngx_command_t *cmd, void *cnf)
{
  Config *conf = static_cast<Config*>(cnf);

  // HANDLE BLOCK HERE
  ngx_pool_t* pool = ngx_create_pool(NGX_DEFAULT_POOL_SIZE, cf->log);
  if (pool == NULL) {
    return reinterpret_cast<char*>(NGX_CONF_ERROR);
  }

  Dictionary* dict = POOL_ALLOC(cf->pool, Dictionary);

  ngx_conf_t save = *cf;
  cf->pool = pool;
  cf->ctx = dict;
  cf->handler = &sdchx_dictionary_param;
  cf->handler_conf = NULL;

  char* rv = ngx_conf_parse(cf, NULL);

  *cf = save;

  ngx_destroy_pool(pool);

  if (rv != NULL)
    return rv;

  if (dict->init()) {
    return NGX_CONF_OK;
  } else {
    return const_cast<char*>("Can't initialize dictionary");
  }

  return NGX_CONF_OK;
}


static char *
merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    Config *prev = static_cast<Config*>(parent);
    Config *conf = static_cast<Config*>(child);
    ngx_http_compile_complex_value_t ccv;

    // TODO(bacek)

    return NGX_CONF_OK;
}


static ngx_int_t
filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = body_filter;

    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "http sdch filter init");
    return NGX_OK;
}

}  // namespace sdch

// It should be outside namespace
ngx_module_t  sdchx_module = {
    NGX_MODULE_V1,
    &sdchx::sdchx_module_ctx,           /* module context */
    sdchx::filter_commands,            /* module directives */
    NGX_HTTP_MODULE,                  /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

