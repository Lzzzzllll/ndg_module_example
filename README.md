# 1. nginx 模块开发实例
## 1.1 Nginx 开发示例 - ndg_hello 模块
- 配置指令、参数
- 如何访问配置参数、如何处理 TCP/HTTP 请求
- 如何编译集成进 Nginx

##### 设计

- 模块名：**ngx_http_ndg_hello_module**
- 配置指令：**ndg_hello on | off**，开关模块功能，只能在 location 里配置
- 不直接处理 HTTP 请求，只在 **URL 重写阶段**里执行
- 根据配置指令的 on|off 决定输出字符串的内容
- 编写 config 脚本，用 `--add-module` 静态链接选项集成进Nginx
##### 编译
假设该模块位于 /home/test/ndg_module_example/ndg_hello 文件夹下：
```shell
./configure \
 --prefix=/opt/nginx \
 --with-stream \
 --with-threads \
 --with-pcre=/home/renzheng/CLionProjects/pcre \
 --with-http_ssl_module --with-http_v2_module \
 --without-http_fastcgi_module \
 --build="renzheng build at `date +%Y%m%d`" \
 --with-debug \
 --add-module=/home/renzheng/CLionProjects/ndg_module_example/ndg_hello
make
sudo make install
```
##### 测试验证
配置参数
```
master_process off;
daemon off;

location /hello {
    ndg_hello on;
}
```
保存重启 nginx 后，使用 curl 访问
```shell
curl -v 'http://localhost/hello'
```
可在控制台或日志中查找到相应字符

## 1.2 Nginx 请求处理

### 1.2.1 ndg_echo 模块
##### 设计
- 模块名：**ngx_http_ndg_echo_module**
- 配置指令：**ndg_echo**，接受一个参数
- 是一个内容处理模块
- 只接受 GET 请求方法
- 使用 content handler 的方式注册处理函数
- 功能：
  - 向客户端输出一个指定的字符串信息（由 ndg_echo 指定）
  - uri 里的参数信息也一并输出
##### 编译
假设该模块位于 /home/test/ndg_module_example/ndg_echo 文件夹下：
```shell
./configure \
 --prefix=/opt/nginx \
 --with-stream \
 --with-threads \
 --with-pcre=/home/renzheng/CLionProjects/pcre \
 --with-http_ssl_module --with-http_v2_module \
 --without-http_fastcgi_module \
 --build="renzheng build at `date +%Y%m%d`" \
 --with-debug \
 --add-module=/home/renzheng/CLionProjects/ndg_module_example/ndg_echo
make
sudo make install
```
##### 测试验证

配置参数
```
location /hello {
    ndg_echo "renz2048 ndg echo module\n";
}
```
在浏览器里访问 http://localhost/hello ，界面显示 `renz2048 ndg echo module`

### 1.2.2 ndg_filter 模块

##### 设计

- 模块名：**ngx_http_ndg_filter_module**
- 配置指令：
  - **ndg_header**：接受多个 keyval 参数，加入到响应头
  - **ndg_footer**：接受一个字符串参数，加入到响应体末尾
- 使用 ctx 记录状态，防止重复添加

##### 编译

```shell
./configure \
 --prefix=/opt/nginx \
 --with-stream \
 --with-threads \
 --with-pcre=/home/renzheng/CLionProjects/pcre \
 --with-http_ssl_module --with-http_v2_module \
 --without-http_fastcgi_module \
 --build="renzheng build at `date +%Y%m%d`" \
 --with-debug \
 --add-module=/home/renzheng/CLionProjects/ndg_module_example/ndg_echo \
 --add-module=/home/renzheng/CLionProjects/ndg_module_example/ndg_filter
make
sudo make install
```

##### 测试验证

配置参数

```
location /filter {
    ndg_hello on;
    ndg_echo "renz2048 ndg echo module\n";
    ndg_header x-name renz2048;
    ndg_header x-value trigger;
    ndg_footer "copyright renz2048!\n";
}
```

`sudo /opt/nginx/sbin/nginx -c /opt/nginx/conf/nginx.conf -s reload` 重启 nginx 服务。

浏览器访问 http://localhost/filter ，界面显示：

```
renz2048 ndg echo module
copyright renz2048!
```

按 F12 ，选择 Network->Headers ，再 Response Headers 中显示：

```
x-name: renz2048
x-value: trigger
```

## 1.3 Nginx 请求转发

### 1.3.1 upstream 模块

##### 设计

- 模块名：ngx_http_ndg_upstream_module;
- 注册处理函数：使用 content handler 方式
- 指令：ndg_upstream_pass 启动转发处理
- upstream 框架上游的连接参数硬编码，不在配置文件里设置
- 仅支持 GET 方法，转发 $args，末尾附加一个`\n` 标记数据的结束
- 以 `\n` 为结束标记解析响应头，把收到的数据原样转发到下游

##### 编译

```shell
./configure \
 --prefix=/opt/nginx \
 --with-stream \
 --with-threads \
 --with-pcre=/home/renzheng/CLionProjects/pcre \
 --with-http_ssl_module --with-http_v2_module \
 --without-http_fastcgi_module \
 --build="renzheng build at `date +%Y%m%d`" \
 --with-debug \
 --add-module=/home/renzheng/CLionProjects/ndg_module_example/ndg_upstream
make
sudo make install
```

测试验证

```
http {
    server {
        location /upstream {
            ndg_upstream_pass backend;
        }
    }
    
    upstream backend {
        server 127.0.0.1:80
    }
}
```

在浏览器访问 http://localhost:8080/upstream?xxx ，通过抓包显示 http 流如下：

```
GET /upstream?xxx HTTP/1.1
Host: localhost
User-Agent: curl/7.68.0
Accept: */*

HTTP/1.1 404 Not Found
Date: Fri, 07 Aug 2020 03:34:54 GMT
Server: Apache/2.4.41 (Ubuntu)
Content-Length: 271
Content-Type: text/html; charset=iso-8859-1

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>404 Not Found</title>
</head><body>
<h1>Not Found</h1>
<p>The requested URL was not found on this server.</p>
<hr>
<address>Apache/2.4.41 (Ubuntu) Server at localhost Port 80</address>
</body></html>
```

## 1.4 Nginx 子请求

### 1.4.1 ndg_subrequest 模块

##### 设计

- 模块名：ngx_http_subrequest_module，工作在 access 阶段
- 指令：ndg_subrequest
  - 用于确定发起子请求使用的 uri，即 location
- 模块检查子请求返回的状态码决定 allow 或 deny

##### 编译

```shell
./configure \
--add-module=/home/renz/work/ndg_module_example/ndg_echo \
--add-module=/home/renz/work/ndg_module_example/ndg_filter \
--add-module=/home/renz/work/ndg_module_example/ndg_upstream \
--add-module=/home/renz/work/ndg_module_example/ndg_subrequest
--with-stream --with-debug --prefix=/opt/nginx
make
sudo make install
```

##### 测试验证

在配置文件 nginx.conf 的 server 里添加以下 location：

```
        location = /access {
            internal;
            proxy_pass http://192.168.10.59;
        }

        location /subrequest {
            ndg_subrequest "/access";
            ndg_echo "hello subrequest\n";
        }
```

重启后使用 curl 访问：

```shell
curl -v 'http://192.168.10.59/subrequest'
```

可在error.log日志里找到记录：

```
ngd subrequest ok, body is ...
```

然而我并没有找到，只有：

```
2020/01/11 16:29:01 [alert] 1610#0: worker process 1611 exited on signal 11 (core dumped)
```

## 1.5 Nginx 变量

```

```

