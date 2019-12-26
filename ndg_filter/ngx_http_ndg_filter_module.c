#include <ngx_http.h>

static void *ngx_http_ndg_filter_create_loc_conf(ngx_conf_t* cf);
static ngx_int_t ngx_http_ndg_filter_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_ndg_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_ndg_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

typedef struct {
    ngx_array_t* headers;          //存储修改响应头的 kv 数据
    ngx_str_t footer;              //存储修改响应体的字符串
} ngx_http_ndg_filter_loc_conf_t;  //配置结构体

static void *ngx_http_ndg_filter_create_loc_conf(ngx_conf_t* cf)
{
    ngx_http_ndg_filter_loc_conf_t* conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ndg_filter_loc_conf_t));
    return conf;
}

/**
 * 0：还未开始处理，需要处理响应头
 * 1：已处理响应头，开始处理响应体
 * 2：响应体已处理完毕
 */
typedef struct {
    int flag;                  //过滤处理的标识位
} ngx_http_ndg_filter_ctx_t;

/**
 * 为什么要用 static
 */
static ngx_command_t ngx_http_ndg_filter_cmds[] =
{
    {
        ngx_string("ndg_header"),//添加头信息的指令
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,//两个参数
        ngx_conf_set_keyval_slot,//Nginx 的解析 kv 参数
        NGX_HTTP_LOC_CONF_OFFSET,//只能在 location 里出现
        offsetof(ngx_http_ndg_filter_loc_conf_t, headers),
        NULL
    },
    {
        ngx_string("ndg_footer"),//添加末尾字符串
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,//一个参数
        ngx_conf_set_str_slot,//Nginx 的解析函数
        NGX_HTTP_LOC_CONF_OFFSET,//只能在 location 里出现
        offsetof(ngx_http_ndg_filter_loc_conf_t, footer),
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_ndg_filter_module_ctx =
{
    NULL,                                 //preconfiguration
    ngx_http_ndg_filter_init,             //postconfiguration
    NULL,                                 //create main configuration
    NULL,                                 //init main configuration
    NULL,                                 //create server configuration
    NULL,                                 //merge server configuration
    ngx_http_ndg_filter_create_loc_conf,  //create location configuration
    NULL,                                 //merge location configuration
};

ngx_module_t ngx_http_ndg_filter_module =//模块定义，不能是静态变量
{
    NGX_MODULE_V1,//标准的填充宏
    &ngx_http_ndg_filter_module_ctx,//函数指针表
    ngx_http_ndg_filter_cmds,//配置指令数值
    NGX_HTTP_MODULE,//http 模块必须的标记
    NULL,//init master
    NULL,//init module
    NULL,//init process
    NULL,//init thread
    NULL,//exit thread
    NULL,//exit process
    NULL,//exit master
    NGX_MODULE_V1_PADDING//标准的填充宏
};

/**
 * 定义两个函数指针，调整函数链
 */
static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static ngx_int_t ngx_http_ndg_filter_init(ngx_conf_t* cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_ndg_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_ndg_body_filter;

    return NGX_OK;
}

/**
 * 过滤响应头
 */
static ngx_int_t ngx_http_ndg_header_filter(ngx_http_request_t *r)
{
    ngx_http_ndg_filter_loc_conf_t* lcf;

    ngx_http_ndg_filter_ctx_t* ctx =            //获取环境数据
            ngx_http_get_module_ctx(r, ngx_http_ndg_filter_module);
    if (ctx == NULL) {                          //没有 ctx 则内存池创建
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_ndg_filter_ctx_t));
        ngx_http_set_ctx(                       //ctx 数据加入请求结构体里
              r, ctx, ngx_http_ndg_filter_module);
    }

    if (ctx->flag > 0) {
        return ngx_http_next_header_filter(r);  //继续后续链表的处理
    }

    lcf = ngx_http_get_module_loc_conf(r,       //获取本模块的配置数据
          ngx_http_ndg_filter_module);
    if (lcf->headers) {
        ngx_keyval_t* data = lcf->headers->elts;//指向数组元素
        ngx_uint_t i;

        for (i = 0; i < lcf->headers->nelts; ++i) {
            ngx_table_elt_t *h = ngx_list_push(&r->headers_out.headers);//加入响应头链表里
            h->hash = 1;
            h->key = data[i].key;
            h->value = data[i].value;
        }

        ctx->flag = 1;
    }

    if (lcf->footer.len && r->headers_out.content_length_n > 0) {
        r->headers_out.content_length_n += lcf->footer.len;
        ctx->flag = 1;
    }

    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_ndg_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_ndg_filter_loc_conf_t* lcf;
    ngx_chain_t *p;
    ngx_buf_t *b;

    if (in == NULL) {                           //响应数据为空
        return ngx_http_next_body_filter(r, in);//继续后续链表的处理
    }

    lcf = ngx_http_get_module_loc_conf(r,       //获取本模块的配置数据
          ngx_http_ndg_filter_module);

    if (lcf->footer.len == 0) {                 //空字符串不添加
        return ngx_http_next_body_filter(r, in);
    }

    ngx_http_ndg_filter_ctx_t* ctx =            //获取环境数据 ctx
          ngx_http_get_module_ctx(r, ngx_http_ndg_filter_module);

    if (ctx == NULL) {                          //没有 ctx 则内存池创建
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_ndg_filter_ctx_t));
        ngx_http_set_ctx(r, ctx, ngx_http_ndg_filter_module);//ctx 数据加入请求结构体中
    }

    if (ctx->flag != 1) {
        return ngx_http_next_body_filter(r, in);
    }

    for (p = in; p; p = p->next) {             //检查响应数据链
        if (p->buf->last_buf) {                //查找数据末尾，即 eof
            break;
        }
    }

    if (p == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx->flag = 2;

    b = ngx_calloc_buf(r->pool);               //创建一块空缓冲区

    b->pos = lcf->footer.data;                 //指向 footer 字符串
    b->last = lcf->footer.data + lcf->footer.len;
    b->memory = 1;                             //缓冲区内数据不可修改
    b->last_buf = 1;                           //是最后一块数据
    b->last_in_chain = 1;                      //也是链表最后一块

    if (ngx_buf_size(p->buf) == 0) {           //数据块链最后是否是空的
        p->buf = b;                            //直接添加进链表
        return ngx_http_next_body_filter(r, in);
    }

    ngx_chain_t *out = ngx_alloc_chain_link(r->pool);//分配一个链表节点
    out->buf = b;                              //添加进链表节点
    out->next = NULL;                          //next 指针必须是 NULL

    p->next = out;                             //链接到原链表末尾
    p->buf->last_buf = 0;                      //修改 last 相关标志位
    p->buf->last_in_chain = 0;

    return ngx_http_next_body_filter(r, in);
}