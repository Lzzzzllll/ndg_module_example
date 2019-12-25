# nginx 模块开发实例
- ndg_hello
- ndg_echo
# ndg_hello 模块
## ndg_hello 模块设计
- 模块名：ngx_http_ndg_hello_module
- 配置指令：ndg_hello on | off，开关模块功能，只能在 location 里配置
- 不直接处理 HTTP 请求，只在 URL 重写阶段里执行
- 根据配置指令的 on|off 决定输出字符串的内容
- 编写 config 脚本，用 `--add-module` 静态链接选项集成进Nginx
## 编译
假设该模块位于 /home/test/ndg_module_example/ndg_hello 文件夹下：
```
./configure \
--add-module=/home/test/ndg_module_example/ndg_hello
make
sudo make install
```
## 测试验证
配置参数
```
master_process off;
daemon off;

location /hello {
    ndg_hello on;
}
```
保存重启 nginx 后，使用 curl 访问
```
curl -v 'http://localhost/hello'
```
可在控制台或日志中查找到相应字符
# ndg_echo 模块
## ndg_echo 模块设计
- 模块名：ngx_http_ndg_echo_module
- 配置指令：ndg_echo，接受一个参数
- 是一个内容处理模块
- 只接受 GET 请求方法
- 使用 content handler 的方式注册处理函数
- 功能：
  - 向客户端输出一个指定的字符串信息（由 ndg_echo 指定）
  - uri 里的参数信息也一并输出
## 编译
假设该模块位于 /home/test/ndg_module_example/ndg_echo 文件夹下：
```
./configure \
--add-module=/home/test/ndg_module_example/ndg_echo
make
sudo make install
```
## 测试验证

配置参数
```
location /hello {
    ndg_echo "hello nginx\n";
}
```
# ndg_filter 模块

## 设计

- 模块名：ngx_http_ndg_filter_module
- 配置指令：
  - ndg_header：接受多个 keyval 参数，加入到响应头
  - ndg_footer：接受一个字符串参数，加入到响应体末尾
- 使用 ctx 记录状态，防止重复添加