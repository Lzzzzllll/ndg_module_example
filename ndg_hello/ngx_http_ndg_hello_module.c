#include <ngx_http.h>

static void *ngx_http_ndg_hello_create_loc_conf(ngx_conf_t* cf);
static ngx_int_t ngx_http_ndg_hello_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_ndg_hello_init(ngx_conf_t* cf);

/**
 * 配置的数据结构体
 */
typedef struct {
    ngx_flag_t enable;             //标志变量
} ngx_http_ndg_hello_loc_conf_t;

/**
 * 解析配置指令
 * 使用 ngx_command_t 和相关解析函数
 */
static ngx_command_t ngx_http_ndg_hello_cmds[] =
{
    {
        ngx_string("ndg_hello"),                   //指令的名字
        NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,           //指令的作用域和类型
        ngx_conf_set_flag_slot,                    //解析函数指针
        NGX_HTTP_LOC_CONF_OFFSET,                  //数据的存储位置
        offsetof(                                  //数据的具体存储变量
            ngx_http_ndg_hello_loc_conf_t, enable),
        NULL
    },
    ngx_null_command                               //空对象，结束数组
};

/**
 * 定义 http 模块的功能函数
 */
static ngx_http_module_t ngx_http_ndg_hello_module_ctx = 
{
    NULL,                                   //解析配置文件前被调用
    ngx_http_ndg_hello_init,                //解析配置文件后被调用
    NULL,                                   //创建 HTTP main 域的配置结构
    NULL,                                   //初始化 HTTP main 域的配置结构
    NULL,                                   //创建 Server 域的配置结构
    NULL,                                   //合并 Server 域的配置结构
    ngx_http_ndg_hello_create_loc_conf,     //创建 location 域的配置结构
    NULL,                                   //合并 location 域的配置结构
};

/**
 * 定义模块的数据结构体
 */
ngx_module_t ngx_http_ndg_hello_module =
{
    NGX_MODULE_V1,                    //标准的填充宏
    &ngx_http_ndg_hello_module_ctx,   //配置功能函数
    ngx_http_ndg_hello_cmds,          //配置指令数组
    NGX_HTTP_MODULE,                  //http 模块必须的 tag
    NULL,                             //init master
    NULL,                             //init module
    NULL,                             //init process
    NULL,                             //init thread
    NULL,                             //exit thread
    NULL,                             //exit process
    NULL,                             //exit master
    NGX_MODULE_V1_PADDING
};

/**
 * 为 ndg_hello 模块配置的数据结构体分配内存
 */
static void *
ngx_http_ndg_hello_create_loc_conf(ngx_conf_t* cf)  //创建配置数据结构
{
    ngx_http_ndg_hello_loc_conf_t* conf;            //配置结构体指针

    conf = ngx_pcalloc(cf->pool,                    //由内存池分配内存
            sizeof(ngx_http_ndg_hello_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    conf->enable = NGX_CONF_UNSET;
    return conf;
}

/**
 * 处理函数
 */
static ngx_int_t
ngx_http_ndg_hello_handler(ngx_http_request_t *r)
{
    ngx_http_ndg_hello_loc_conf_t* lcf;                  //配置结构体指针
    lcf = ngx_http_get_module_loc_conf(                  //获取配置数据
        r, ngx_http_ndg_hello_module);
    if (lcf->enable) {
        printf("hello nginx\n");
        ngx_log_error(NGX_LOG_ERR, r->connection->log,
                       0, "hello ansi c");
    } else {
        printf("hello disabled\n");
    }

    return NGX_DECLINED;
}

/**
 * 注册函数
 */
static ngx_int_t
ngx_http_ndg_hello_init(ngx_conf_t* cf)
{
    ngx_http_handler_pt *h;                             //处理函数指针
    ngx_http_core_main_conf_t *cmcf;                    //http 核心配置
    cmcf = ngx_http_conf_get_module_main_conf(
        cf, ngx_http_core_module);

    h = ngx_array_push(
        &cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    *h = ngx_http_ndg_hello_handler;

    return NGX_OK;
}