# gany-tools

中文 | [English](./README_EN.md)

GAny 工具集，包含用于 C++ 代码生成和文档生成的实用工具。

## 项目构建

### 环境要求

- CMake 3.20 或更高版本
- C++20 支持的编译器
- Git（用于获取依赖）

### 构建步骤

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

构建完成后，可执行文件将输出到 `build/bin` 目录。

## 工具说明

### autoany

`autoany` 是一个 C++ 反射代码自动生成工具，通过解析源代码中的注释标签，自动生成类、结构体和枚举的反射代码，用于 GAny 类型系统。

#### 使用方法

```bash
autoany [options] -m "ModuleName" -b "include/path" -p "prefix/" -o "./output/" file1.h file2.h
```

#### 命令行参数

| 参数 | 简写 | 说明 |
|------|------|------|
| `--help` | `-h` | 显示帮助信息 |
| `--module-name=string` | `-m` | 指定生成的模块名称（必需） |
| `--base-path=string` | `-b` | 基础路径，需要解析的头文件的起始路径（必需） |
| `--include-prefix=string` | `-p` | 生成代码时包含源文件的前缀 |
| `--output=string` | `-o` | 输出路径（必需） |

#### 使用示例

```bash
autoany -m "Gx" -b "include/gx" -p "gx/" -o "./gx/toany/" \
    include/gx/vector.h \
    include/gx/matrix.h \
    include/gx/transform.h
```

这将：
1. 解析指定的头文件
2. 为每个头文件生成 `ref_*.cpp` 反射代码文件
3. 生成一个 `reg_Gx.cpp` 模块注册文件

#### 文档标签

在 C++ 头文件中使用以下文档标签来标记需要反射的类型：

##### 命名空间相关

- `@using_ns [namespace]` - 指示需要 using 一个命名空间
- `@ns [name]` - 表示类/枚举反射后的命名空间
- `@cpp_ns [name]` - C++ 命名空间

##### 类型定义

- `@class [name]` - 标记类需要反射
- `@struct [name]` - 标记结构体需要反射
- `@enum [name]` - 标记枚举需要反射
- `@inherit [name]` - 指定类继承的基类（可多个）
- `@enum_item [name]` - 枚举项（每行一个）
- `@cast_to [type]` - 枚举值需要类型转换到指定类型
- `@cpp_name [name]` - 列出枚举类的 C++ 类型名称
- `@def_enum` - DEF_ENUM_N 和 DEF_ENUM_FLAGS_N 的专用标签

##### 成员相关

- `@construct` - 标记构造函数
- `@default_construct` - 标记存在默认构造函数
- `@func [name]` - 标记成员函数（名称可选）
- `@static_func [name]` - 标记静态成员函数
- `@meta_func [name]` - 标记元函数，对应 MetaFunction 枚举值（如：ToString）
- `@property_get [name]` - 标记属性的 getter 函数
- `@property_set [name]` - 标记属性的 setter 函数
- `@property [name]` - 标记成员属性
- `@constant [name]` - 标记常量
- `@pack_again [type]` - 重新打包属性，将使用 REF_PROPERTY_RW

##### 其他

- `@include_from [path]` - 指定在前置声明中需要导入的头文件
- `@ref_code` - 自定义反射代码块，写在 @ref_code 后换行，支持多行，将原样插入到反射函数中
- `@alias` - 类型别名，例如：`@alias NewName = OldName`

#### 代码示例

```cpp
// example.h

/// @ns Math
/// @cpp_ns math
namespace math
{
/**
 * @class Vector3
 */
class Vector3 
{
public:
    /**
     * @default_construct
     */
    
    /**
     * @construct
     */
    Vector3(float x, float y, float z);
    
    /**
     * @property x
     */
    float x;
    
    /**
     * @property y
     */
    float y;
    
    /**
     * @property z
     */
    float z;
    
    /**
     * @func length
     */
    float length() const;
    
    /**
     * @static_func zero
     */
    static Vector3 zero();
};

/**
 * @enum Color
 * @enum_item Red
 * @enum_item Green
 * @enum_item Blue
 */
enum class Color {
    Red,
    Green,
    Blue
};
}
```

#### 输出文件

工具会生成以下文件：

1. **ref_[源文件名].cpp** - 每个源文件对应的反射代码
   - 包含类型的反射注册函数
   - 自动生成属性、方法的绑定代码

2. **reg_[模块名].cpp** - 模块注册文件
   - 调用所有 ref_*.cpp 中的注册函数
   - 使用 REGISTER_GANY_MODULE 宏注册模块

#### 模块头文件

在使用 `autoany` 之前，需要预先创建一个模块头文件 `reg_<ModuleName>.h`，用于声明 GAny 模块。

例如，对于模块名称 `Gx`，需要创建 `reg_Gx.h`：

```cpp
#ifndef GX_REG_GX_H
#define GX_REG_GX_H

#include "gx/gany_module_def.h"

GANY_MODULE_DEFINE(Gx);

#endif //GX_REG_GX_H
```

该头文件的作用：
- 使用 `GANY_MODULE_DEFINE` 宏声明模块
- 模块名称需要与 `-m` 参数指定的名称一致
- `autoany` 生成的 `reg_<ModuleName>.cpp` 会包含此头文件

#### 注意事项

1. 必须先创建 `reg_<ModuleName>.h` 模块头文件
2. 所有输入文件必须在 `--base-path` 指定的目录下
3. 以 `reg_` 开头的源文件会被自动跳过
4. 输出目录不存在时会自动创建
5. 生成的代码依赖 GAny 库

### doc_make

`doc_make` 是一个文档自动生成工具，可以从 GAny 导出的模块中提取类型信息，生成多种格式的接口文档，包括 Markdown、EmmyLua、JSON 和 JavaScript。

#### 使用方法

```bash
doc_make -p [work_path] -o [output_path] -t [doc_type] [module_file_a] [module_file_b]
```

#### 命令行参数

| 参数 | 简写 | 说明 |
|------|------|------|
| `--help` | `-h` | 显示帮助信息 |
| `--path=string` | `-p` | 工作目录路径 |
| `--type=md\|lua\|js\|json\|all` | `-t` | 生成的文档类型（可选） |
| `--output=string` | `-o` | 输出路径 |

#### 支持的文档类型

- `md` - Markdown 格式文档
- `lua` - EmmyLua 注解格式文档（用于 Lua IDE 代码提示）
- `json` - JSON 格式的结构化文档
- `js` - JavaScript 格式文档
- `all` - 生成以上所有格式的文档

如果不指定 `-t` 参数，默认生成 Markdown 格式文档。

#### 使用示例

##### 生成 Markdown 文档

```bash
doc_make -p ./modules -o ./docs -t md libMyModule.so
```

##### 生成 EmmyLua 文档

```bash
doc_make -p ./modules -o ./docs -t lua libMyModule.so
```

##### 生成所有格式文档

```bash
doc_make -p ./modules -o ./docs -t all libMyModule.so libAnotherModule.so
```

##### 生成多个模块的文档

```bash
doc_make -p ./ -o ./api-docs module1.dll module2.dll module3.dll
```

#### 输出结构

工具会按照类的命名空间组织输出文档：

```
output/
├── namespace1/
│   ├── ClassA.md
│   ├── ClassA.lua
│   ├── ClassA.json
│   └── ClassA.js
├── namespace2/
│   ├── ClassB.md
│   └── ClassB.lua
└── GLOBAL/
    └── GlobalClass.md
```

- 每个命名空间对应一个子目录
- 没有命名空间的类放在 `GLOBAL` 目录下
- 每个类生成对应格式的文档文件

#### 工作流程

1. **加载模块** - 从指定的模块文件（.so/.dll/.dylib）中加载 GAny 模块
2. **提取类型信息** - 通过 GAny 反射系统获取所有注册的类信息
3. **生成文档** - 根据指定的文档类型生成相应格式的文档
4. **组织输出** - 按命名空间结构组织输出文件

#### 文档内容

生成的文档会包含：

- 类的完整名称和命名空间
- 继承关系
- 构造函数
- 成员属性（包括类型和访问权限）
- 成员方法（包括参数、返回值类型）
- 静态方法
- 常量定义
- 枚举类型及其取值

#### 注意事项

1. 模块文件必须是通过 GAny 系统导出的动态库
2. 输出目录不存在时会自动创建
3. 可以同时处理多个模块文件
4. 生成的文档基于 GAny 反射注册的信息，未注册的类型不会出现在文档中
5. 工作目录路径用于定位模块文件的相对路径

## 依赖项

项目通过 Git 自动获取以下依赖：

- [gany](https://github.com/giarld/gany) - GAny 类型系统核心库
- [gx](https://github.com/giarld/gx) - GX 基础库

## 许可证

[LICENSE](./LICENSE)
