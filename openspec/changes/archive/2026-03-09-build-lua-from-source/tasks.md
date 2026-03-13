## 1. 下载并引入 Lua 5.4 源码

- [x] 1.1 下载 Lua 5.4.7 源码
- [x] 1.2 将 `src/` 目录内容复制到项目的 `lua/` 目录

## 2. 创建 CMake 构建配置

- [x] 2.1 创建根目录 `CMakeLists.txt`，定义 liblua 静态库、lua 和 luac 可执行文件

## 3. 创建项目配置文件

- [x] 3.1 创建 `.gitignore`，排除 `build/` 目录
- [x] 3.2 创建 `README.md`，包含构建说明

## 4. 创建测试脚本

- [x] 4.1 创建 `test/hello.lua` 测试脚本

## 5. 编译验证

- [x] 5.1 运行 CMake 配置和编译
- [x] 5.2 使用编译出的 lua.exe 运行测试脚本，验证输出正确