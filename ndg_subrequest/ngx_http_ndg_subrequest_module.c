#include <ngx_http.h>
#include <nginx.h>

static void *ngx_http_ndg_subrequest_create_loc_conf(ngx_conf_t* cf);
static char *ngx_http_ndg_subrequest_merge_loc_conf(ngx_conf_t*cf, void* parent, void* child);
static ngx_int_t ngx_http_ndg_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);
static ngx_int_t ngx_http_ndg_subrequest_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_ndg_subrequest_handler(ngx_http_request_t *r);


/**
 * 定义模块配置信息
 */
typedef struct {
    ngx_str_t uri;
} ngx_http_ndg_subrequest_loc_conf_t;

/**
 * 在 ctx 里存储子请求的输出数据
 */
typedef struct {
    ngx_http_request_t *sr;
} ngx_http_ndg_subrequest_ctx_t;

static ngx_command_t ngx_http_ndg_subrequest_cmds[] =
{
    {
        ngx_string("ndg_subrequest"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_ndg_subrequest_loc_conf_t, uri),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_ndg_subrequest_module_ctx =
{
    NULL,
    ngx_http_ndg_subrequest_init,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_ndg_subrequest_create_loc_conf,
    ngx_http_ndg_subrequest_merge_loc_conf,
};

ngx_module_t ngx_http_ndg_subrequest_module =
{
    NGX_MODULE_V1,
    &ngx_http_ndg_subrequest_module_ctx,
    ngx_http_ndg_subrequest_cmds,
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

static void *ngx_http_ndg_subrequest_create_loc_conf(
         ngx_conf_t* cf)
{
    ngx_http_ndg_subrequest_loc_conf_t* conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ndg_subrequest_loc_conf_t));
    return conf;
}

static char *ngx_http_ndg_subrequest_merge_loc_conf(
         ngx_conf_t*cf, void* parent, void* child)
{
    ngx_http_ndg_subrequest_loc_conf_t* prev = parent;
    ngx_http_ndg_subrequest_loc_conf_t* conf = child;

    ngx_conf_merge_str_value(conf->uri, prev->uri, "");
    return NGX_CONF_OK;
}

/**
 * 子请求回调函数
 * 将子请求指针放置在本模块的 ctx 里，回传给父请求
 */
static ngx_int_t
ngx_http_ndg_subrequest_post_handler(
    ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_request_t *pr;
    ngx_http_ndg_subrequest_ctx_t *ctx;
    pr = r->parent;//获取父请求
    pr->write_event_handler = ngx_http_core_run_phases;//设置付请求的回调函数
    ctx = ngx_http_get_module_ctx(
             pr, ngx_http_ndg_subrequest_module);
    if (ctx == NULL) {
        ctx = ngx_pcalloc(pr->pool, sizeof(ngx_http_ndg_subrequest_ctx_t));
        ngx_http_set_ctx(pr, ctx, ngx_http_ndg_subrequest_module);
    }
    ctx->sr = r;

    return NGX_OK;
}

/**
 * 注册函数
 * 为了在 access 阶段工作
 * 在 ngx_http_core_module 的 phases 数组添加处理函数
 */
static ngx_int_t ngx_http_ndg_subrequest_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    *h = ngx_http_ndg_subrequest_handler;

    return NGX_OK;
}

/**
 * 处理函数
 * 若 ctx 为空，发起子请求；否则检查子请求结束后返回的数据
 */
static ngx_int_t ngx_http_ndg_subrequest_handler(
         ngx_http_request_t *r)
{
    ngx_http_ndg_subrequest_loc_conf_t *lcf;
    ngx_http_ndg_subrequest_ctx_t *ctx;
    ngx_http_post_subrequest_t *psr;
    ngx_http_request_t *sr;
    ngx_int_t rc;
    ngx_str_t str;

    lcf = ngx_http_get_module_loc_conf(
             r, ngx_http_ndg_subrequest_module);
    if (lcf->uri.len == 0) {
        return NGX_DECLINED;//空字符串不处理
    }

    ctx = ngx_http_get_module_ctx(
             r, ngx_http_ndg_subrequest_module);
    if (ctx == NULL) {//空则需要发起子请求
        psr = ngx_pcalloc(
                 r->pool, sizeof(ngx_http_post_subrequest_t));
        //子请求回调函数对象
        psr->handler = ngx_http_ndg_subrequest_post_handler;
        sr = ctx->sr;
        rc = ngx_http_subrequest(r, &lcf->uri, &r->args, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);
        if (rc != NGX_OK) {
            return NGX_ERROR;
        }

        return NGX_DONE;
    }

    sr = ctx->sr;
    str.data = sr->out->buf->pos;
    str.len = ngx_buf_size(sr->out->buf);
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ndg_subrequest ok, body is %V", &str);

    return sr->headers_out.status == NGX_HTTP_OK ? NGX_OK:NGX_HTTP_FORBIDDEN;
}