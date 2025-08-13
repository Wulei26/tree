## Linux Tree Command Implementation in C

功能：以树状结构递归显示目录内容，模拟Linux tree命令的核心功能

编译方法：
```shell
g++ -o tree tree.c
```
使用方法：
```shell
./tree [选项] [目录]
```
**选项说明：**

`-a, --all         显示所有文件，包括隐藏文件（以.开头的文件）`

`-d, --dir-only    只显示目录，不显示文件`

`-f, --full-path   显示文件的完整路径`

`-L, --level N     限制递归深度为N层`

`-h, --help        显示帮助信息`

**使用示例：**

1. 显示当前目录的树状结构：
    `./tree`
 
2. 显示指定目录的树状结构：
       `./tree /path/to/directory`
    
3. 显示所有文件（包括隐藏文件）：
         `./tree -a`
    
4. 只显示目录：
         `./tree -d`
    
5. 显示完整路径：
         `./tree -f`
    
6. 限制递归深度为2层：
         `./tree -L 2`
    
7. 组合使用选项：
         `./tree -a -L 3 /home/user`
