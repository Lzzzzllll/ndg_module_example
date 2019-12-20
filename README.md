# ndg_hello 模块设计
- 模块名：ngx_http_ndg_hello_module
- 配置指令：ndg_hello on | off，开关模块功能，只能在 location 里配置
- 不直接处理 HTTP 请求，只在 URL 重写阶段里执行
- 根据配置指令的 on|off 决定输出字符串的内容
- 编写 config 脚本，用 `--add-module` 静态链接选项集成进Nginx
# 编译
假设该模块位于 /home/test/ndg_hello 文件夹下：
```
./configure --add-module=/home/test/ndg_hello
make
sudo make install
```
# 测试验证
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