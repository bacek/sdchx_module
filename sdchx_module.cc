#include <assert.h>

#include "sdchx_module.h"

#include "sdchx_config.h"
#include "sdchx_dictionary.h"
#include "sdchx_pool_alloc.h"
#include "sdchx_request_context.h"
#include "sdchx_main_config.h"
#include "sdchx_output_handler.h"
#include "sdchx_dictionary_metadata_handler.h"

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

    { ngx_string("sdchx_webworker_mode"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF
                        |NGX_HTTP_LIF_CONF
                        |NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(Config, webworker_mode),
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

template<typename T>
class ListForwardIterator : public std::iterator<std::forward_iterator_tag, T> {
 public:
  typedef typename std::iterator<std::forward_iterator_tag, T> base;
  typedef typename base::pointer pointer;
  typedef typename base::reference reference;
  typedef ListForwardIterator<T> this_t;

  ListForwardIterator() : part_(NULL), idx_(0) {}
  explicit ListForwardIterator(ngx_list_t& lst) : part_(&lst.part), idx_(0) {
    if (part_->nelts == 0)
      part_ = NULL;
  }

  friend void swap(this_t& a, this_t& b) {
    std::swap(a.part_, b.part_);
    std::swap(a.idx_,  b.idx_);
  }

  reference operator*() { return static_cast<T*>(part_->elts)[idx_]; }
  pointer operator->() { return &**this; }

  this_t& operator++() {
    ++idx_;
    if (idx_ >= part_->nelts) {
      part_ = part_->next;
      idx_ = 0;
    }
    return *this;
  }
  this_t operator++(int) {
    this_t tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==(const this_t& rhs) const {
    return part_ == rhs.part_ && idx_ == rhs.idx_;
  }
  bool operator!=(const this_t& rhs) const { return !(*this == rhs); }

 private:
  ngx_list_part_t* part_;
  ngx_uint_t idx_;
};

class Table {
 public:
  typedef ListForwardIterator<ngx_table_elt_t> iterator;

  explicit Table(ngx_list_t& lst) : lst_(&lst) {}

  inline iterator begin() { return iterator(*lst_); }
  inline iterator end() { return iterator(); }

 private:
  Table();

  ngx_list_t* lst_;
};


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
                   "sdchx header: not enabled");

    return false;
  }

  if (r->headers_out.status != NGX_HTTP_OK) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdchx header: unsupported status");

    return false;
  }

  if (r->headers_out.content_encoding
    && r->headers_out.content_encoding->value.len) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdchx header: content is already encoded");
    return false;
  }

#if 0
  if (r->headers_out.content_length_n != -1
    && r->headers_out.content_length_n < conf->min_length) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdchx header: content is too small");
    return false;
  }

  if (ngx_http_test_content_type(r, &conf->types) == NULL) {
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "sdchx header: unsupported content type");
    return false;
  }
#endif

  return true;
}

static ngx_int_t
header_filter(ngx_http_request_t *r)
{
  ngx_log_debug(
      NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "http sdchx filter header 000");

  Config* conf = Config::get(r);

  const char *ae_header = conf->webworker_mode 
                          ? "x-accept-encoding"
                          : "accept-encoding";

  ngx_str_t val;
  if (header_find(&r->headers_in.headers, ae_header, &val) == 0 ||
      ngx_strstrn(val.data, const_cast<char*>("sdchx"), val.len) == 0) { // XXX
    ngx_log_debug(NGX_LOG_DEBUG_HTTP,
                  r->connection->log,
                  1,
                  "sdchx header: no 'sdchx' in %s", ae_header);
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

  RequestContext* ctx = POOL_ALLOC(r, RequestContext, r);
  if (ctx == NULL) {
    return NGX_ERROR;
  }

  // Allocate Handlers chain in reverse order
  // Last will be OutputHandler.
  ctx->handler = POOL_ALLOC(r, OutputHandler, ctx, ngx_http_next_body_filter);
  if (ctx->handler == NULL)
    return NGX_ERROR;

  std::string url = to_string(r->uri);
  Dictionary *serving_dictionary =
      conf->dictionary_factory.get_dictionary_by_url(url);
  ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                "sdchx serving_dictionary %p", serving_dictionary);
  if (serving_dictionary) {
    ctx->handler = POOL_ALLOC(r, DictionaryMetadataHandler, serving_dictionary,
                              ctx->handler);
  }

  std::vector<std::string> available_dictionaries;
  Table h(r->headers_in.headers);

  ngx_str_t header = ngx_string("SDCHx-Avail-Dictionary");

  for (Table::iterator i = h.begin(); i != h.end(); ++i) {
    if (i->key.len == header.len &&
          ngx_strncasecmp(i->key.data, header.data, header.len) == 0) {
      available_dictionaries.push_back(to_string(i->value));
    }
  }

  Dictionary *used_dictionary =
      conf->dictionary_factory.select_dictionary(available_dictionaries);
  if (used_dictionary) {
    ctx->handler = used_dictionary->create_handler(ctx, ctx->handler);
  }

  for (Handler* h = ctx->handler; h; h = h->next()) {
    if (!h->init(ctx)) {
      ctx->done = true;
      return NGX_ERROR;
    }
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
      ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0, "sdchx failed");
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

  if (dict->init(cf->pool)) {
    conf->dictionary_factory.add_dictionary(dict);
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

    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "http sdchx filter init");
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

