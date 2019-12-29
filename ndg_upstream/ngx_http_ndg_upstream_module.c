#include <ngx_http.h>

static void *ngx_http_ndg_upstream_create_loc_conf(ngx_conf_t* cf);
static void *ngx_http_ndg_upstream_merge_loc_conf(ngx_conf_t* cf, void *parent, void *child);
static ngx_int_t *ngx_http_ndg_upstream_create_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_ndg_upstream_reinit_request(ngx_http_request_t *r);
static ngx_int_t ngx_http_ndg_upstream_process_header(ngx_http_request_t *r);
static ngx_int_t ngx_http_ndg_upstream_handler(ngx_http_request_t *r);
static char *ngx_http_ndg_upstream_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct {
    ngx_http_upstream_conf_t upstream;//upstream 配置参数
} ngx_http_ndg_upstream_loc_conf_t;

static ngx_command_t ngx_http_ndg_upstream_cmds[] =
{
    {
        ngx_string("ndg_upstream_pass"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_ndg_upstream_pass,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_ndg_upstream_module_ctx =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_ndg_upstream_create_loc_conf,
    ngx_http_ndg_upstream_merge_loc_conf,
};

ngx_module_t ngx_http_ndg_upstream_module =
{
    NGX_MODULE_V1,
    &ngx_http_ndg_upstream_module_ctx,
    ngx_http_ndg_upstream_cmds,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void *ngx_http_ndg_upstream_create_loc_conf(ngx_conf_t* cf)
{
    ngx_http_ndg_upstream_loc_conf_t* conf;//配置结构体指针
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ndg_upstream_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    //四个关键字段初始化为 unset
    conf->upstream.connect_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.send_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.read_timeout = NGX_CONF_UNSET_MSEC;
    conf->upstream.buffer_size = NGX_CONF_UNSET_SIZE;

    return conf;
}

static void *ngx_http_ndg_upstream_merge_loc_conf(ngx_conf_t* cf,
         void *parent, void *child)
{
    ngx_http_ndg_upstream_loc_conf_t* prev = parent;
    ngx_http_ndg_upstream_loc_conf_t* conf = child;

    ngx_conf_merge_msec_value(
             conf->upstream.connect_timeout,
             prev->upstream.connect_timeout,60000);
    ngx_conf_merge_msec_value(
             conf->upstream.send_timeout,
             prev->upstream.send_timeout,10000);
    ngx_conf_merge_msec_value(
             conf->upstream.read_timeout,
             prev->upstream.read_timeout,10000);
    ngx_conf_merge_size_value(
             conf->upstream.buffer_size,
             prev->upstream.buffer_size,(size_t) ngx_pagesize);
    return NGX_CONF_OK;
}

/**
 * 引用 $args，把缓冲区链表挂到 r->upstream->request_bufs 转发到上游
 */
static ngx_int_t *ngx_http_ndg_upstream_create_request(ngx_http_request_t *r)
{
    ngx_str_t lf = ngx_string("\n");//换行符常量

    ngx_buf_t *b1 =ngx_calloc_buf(r->pool);//两个缓冲区
    ngx_buf_t *b2 = ngx_calloc_buf(r->pool);

    b1->pos = r->args.data;//b1 直接指向 $args
    b1->last = r->args.data + r->args.len;

    b2->pos = lf.data;//b2 指向换行符字符串
    b2->last = lf.data + lf.len;
    b2->last_buf = 1;//标记为最后一块数据

    ngx_chain_t *out1 = ngx_alloc_chain_link(r->pool);
    ngx_chain_t *out2 = ngx_alloc_chain_link(r->pool);

    out1->buf = b1;
    out1->next = out2;
    out2->buf = b2;
    out2->next = NULL;

    r->upstream->request_bufs = out1;

    return NGX_OK;
}

static ngx_int_t ngx_http_ndg_upstream_reinit_request(ngx_http_request_t *r)
{
    return NGX_OK;
}

/**
 * 处理上游响应
 */
static ngx_int_t
ngx_http_ndg_upstream_process_header(ngx_http_request_t *r)
{
    ngx_http_upstream_t *u = r->upstream;

    u_char *p = ngx_strlchr(u->buffer.pos, u->buffer.last, '\n');
    if (p == NULL) {
        return NGX_AGAIN;
    }

    u->headers_in.content_length_n = p - u->buffer.pos;
    u->headers_in.status_n = NGX_HTTP_OK;
    u->state->status = NGX_HTTP_OK;

    return NGX_OK;
}

static void ngx_http_ndg_upstream_finalize_request(){}

/**
 * 启动转发
 */
static ngx_int_t ngx_http_ndg_upstream_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_http_upstream_t *u;
    ngx_http_ndg_upstream_loc_conf_t *lcf;

    if (!(r->method & NGX_HTTP_GET)) {
        return NGX_HTTP_NOT_ALLOWED;
    }
    rc = ngx_http_discard_request_body(r);

    if (ngx_http_upstream_create(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    u = r->upstream;
    lcf = ngx_http_get_module_loc_conf(r, ngx_http_ndg_upstream_module);
    u->conf = &lcf->upstream;

    //指定四个必须的回调函数指针
    u->create_request = ngx_http_ndg_upstream_create_request;
    u->reinit_request = ngx_http_ndg_upstream_reinit_request;
    u->process_header = ngx_http_ndg_upstream_process_header;
    u->finalize_request = ngx_http_ndg_upstream_finalize_request;

    r->main->count++;
    ngx_http_upstream_init(r);

    return NGX_DONE;
}

static char *ngx_http_ndg_upstream_pass(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_url_t u;
    ngx_str_t *value;
    ngx_http_ndg_upstream_loc_conf_t *lcf = conf;
    ngx_http_core_loc_conf_t *clcf;

    ngx_memzero(&u, sizeof(ngx_url_t));

    value = cf->args->elts;
    u.url = value[1];
    u.no_resolve = 1;

    lcf->upstream.upstream = ngx_http_upstream_add(cf, &u, 0);
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_ndg_upstream_handler;

    return NGX_CONF_OK;
}