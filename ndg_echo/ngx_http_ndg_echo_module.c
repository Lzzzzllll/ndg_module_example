#include <ngx_http.h>

static ngx_int_t ngx_http_ndg_echo();

typedef struct {
    ngx_str_t msg;      //存储配置文件里的字符串
} ngx_http_ndg_echo_loc_conf_t;     //配置结构体

/**
 * 模块的指令数组定义
 */
static ngx_command_t ngx_http_ndg_echo_cmds[] =
{
    {
        ngx_string("ndg_echo"),     //指令的名字
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,       //指令的作用域和类型
        ngx_http_ndg_echo,      //解析函数
        NGX_HTTP_LOC_CONF_OFFSET,       //只能在 location 里出现
        offsetof(       //解析使用的字段偏移量
            ngx_http_ndg_echo_loc_conf_t, msg),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_ndg_echo_module_ctx =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_ndg_echo_create_loc_conf,
    NULL,
};

ngx_module_t ngx_http_ndg_echo_module =
{
    NGX_MODULE_V1,
    &ngx_http_ndg_echo_module_ctx,
    ngx_http_ndg_echo_cmds,
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

static void *
ngx_http_ndg_echo_create_loc_conf(ngx_conf_t* cf)
{
    ngx_http_ndg_echo_loc_conf_t* conf;  //配置结构体指针

    conf = ngx_pcalloc(cf->pool,        //内存池创建结构体对象
            sizeof(ngx_http_ndg_echo_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    return conf;
}

static ngx_int_t
ngx_http_ndg_echo_handler(ngx_http_request_t *r)
{
    size_t len;
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t *out;
    ngx_http_ndg_echo_loc_conf_t *lcf;

    if (!(r->method & NGX_HTTP_GET)) {  //检查请求方法
        return NGX_HTTP_NOT_ALLOWED;    //要求必须是 GET
    }

    rc = ngx_http_discard_request_body(r);
    lcf = ngx_http_get_module_loc_conf(r, ngx_http_ndg_echo_module);

    len = lcf->msg.len;
    if (r->args.len) {
        len += r->args.len + 1;
    }
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = len;
    rc = ngx_create_temp_buf(r->pool, len);     //分配缓冲区

    if (r->args.len) {
        b->last = ngx_cpymem(b->pos, r->args.data, r->args.len);
        *(b->last++) = ',';
    }
    b->last = ngx_cpymem(b->last, lcf->msg.data, lcf->msg.len);

    b->last_buf = 1;
    b->last_in_chain = 1;

    out = ngx_alloc_chain_link(r->pool);
    out->buf = b;
    out->next = NULL;
    return ngx_http_output_filter(r, out);
}

static ngx_int_t
ngx_http_ndg_echo(ngx_conf_t* cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t* clcf;
    char* rc = ngx_conf_set_str_slot(cf, cmd, conf);
    if (rc != NGX_CONF_OK) {
        return rc;
    }

    clcf = ngx_http_conf_get_module_loc_conf(
            cf, ngx_http_core_module);
    clcf->handler = ngx_http_ndg_echo_handler;

    return NGX_CONF_OK;
}