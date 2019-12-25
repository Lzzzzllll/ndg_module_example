#include <ngx_http.h>

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
void ngx_http_ndg_header_filter()
{
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
        ngx_keyval_t* data = lcf->headers->nelts;//指向数组元素

        for (i = 0; i < lcf->headers->nelts; ++i) {
            *h = ngx_list_push(&r->headers_out.headers);//加入响应头链表里
            h->hash = 1;
            
        }
    }
}