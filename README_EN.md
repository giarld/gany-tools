# gany-tools

[中文](./README.md) | English

GAny toolset containing utilities for C++ code generation and documentation generation.

## Building

### Requirements

- CMake 3.20 or higher
- C++20 compatible compiler
- Git (for fetching dependencies)

### Build Steps

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

After building, executables will be output to the `build/bin` directory.

## Tools

### autoany

`autoany` is a C++ reflection code generator that automatically generates reflection code for classes, structures, and enumerations by parsing documentation tags in source code, for use with the GAny type system.

#### Usage

```bash
autoany [options] -m "ModuleName" -b "include/path" -p "prefix/" -o "./output/" file1.h file2.h
```

#### Command Line Arguments

| Argument | Short | Description |
|----------|-------|-------------|
| `--help` | `-h` | Show help information |
| `--module-name=string` | `-m` | Specify the module name to generate (required) |
| `--base-path=string` | `-b` | Base path, the starting path of header files to parse (required) |
| `--include-prefix=string` | `-p` | Prefix for source files when generating code |
| `--output=string` | `-o` | Output path (required) |

#### Usage Examples

```bash
autoany -m "Gx" -b "include/gx" -p "gx/" -o "./gx/toany/" \
    include/gx/vector.h \
    include/gx/matrix.h \
    include/gx/transform.h
```

This will:
1. Parse the specified header files
2. Generate `ref_*.cpp` reflection code files for each header file
3. Generate a `reg_Gx.cpp` module registration file

#### Documentation Tags

Use the following documentation tags in C++ header files to mark types that need reflection:

##### Namespace Related

- `@using_ns [namespace]` - Indicates the need to use a namespace
- `@ns [name]` - Represents the namespace after class/enum reflection
- `@cpp_ns [name]` - C++ namespace

##### Type Definitions

- `@class [name]` - Mark class for reflection
- `@struct [name]` - Mark struct for reflection
- `@template_type [name = type]` - Instantiate a reflected template class, e.g. `@template_type Vec3f = Vector<float, 3>`; declared members are reused for every template instance
- `@enum [name]` - Mark enum for reflection. Supports `enum`, `enum class`, `DEF_ENUM*`, and `DEF_ENUM_FLAGS*`
- `@inherit [name]` - Specify base class inheritance (multiple allowed)
- Template base classes may be defined in other headers passed to the same `autoany` run; generated code reuses template base members and expands the template base's own non-template parents onto the derived class.
- When a template class inherits another template class, keep the template angle brackets in `@inherit` even if no concrete arguments are written, for example `@inherit BBoxBase<>`; otherwise it is treated as a normal base class and template base members are not reused.
- `@cast_to [type]` - Enum value needs type conversion to specified type
- `@cpp_name [name]` - List the C++ type name of the enum class

##### Member Related

- `@construct` - Mark constructor
- `@default_construct` - Mark existence of default constructor
- `@func [name]` - Mark member function (name optional)
- `@static_func [name]` - Mark static member function
- `@meta_func [name]` - Mark meta function, corresponding to MetaFunction enum value (e.g., ToString)
- `@property_get [name]` - Mark property getter function; the same function can declare multiple getter tags for multiple properties; `@alias` on the function only generates function aliases
- `@property_set [name]` - Mark property setter function; the same function can declare multiple setter tags for multiple properties; `@alias` on the function only generates function aliases
- `@property [name]` - Mark member property
- `@constant [name]` - Mark constant
- `@pack_again [type]` - Repack property, will use REF_PROPERTY_RW

##### Others

- `@include_from [path]` - Specify header files to import in forward declarations
- `@ref_code` - Custom reflection code block, write after @ref_code on new lines, supports multiple lines, will be inserted into reflection function as-is
- `@alias` - Type alias or function/property alias, e.g., `@alias NewName = OldName` or `@alias AliasName`

#### Code Example

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
 */
enum class Color {
    Red,
    Green,
    Blue
};
}
```

##### Template Type Example

```cpp
// template_example.h

/// @ns Math
/// @cpp_ns math
namespace math
{
/**
 * @struct BBox
 */
template<typename T, int N = 1>
struct BBox
{
    /**
     * @default_construct
     */

    /**
     * @property value
     */
    T value;

    /**
     * @func size
     */
    int size() const;
};

/**
 * @struct IntBox
 * @inherit BBox<int>
 */
struct IntBox : BBox<int>
{
    /**
     * @default_construct
     */

    /**
     * @func
     */
    int getValue() const
    {
        return value;
    }
};

/**
 * @struct Vec3f
 * @inherit BBox<float, 3>
 */
struct Vec3f : BBox<float, 3>
{
    /**
     * @func
     */
    std::string toString() const
    {
        std::stringstream ss;
        ss << "{" << value << ", " << value << ", " << value << "}";
        return ss.str();
    }
};
}
```

#### Output Files

The tool will generate the following files:

1. **ref_[source_filename].cpp** - Reflection code for each source file
   - Contains type reflection registration functions
   - Auto-generated property and method binding code

2. **reg_[ModuleName].cpp** - Module registration file
   - Calls registration functions from all ref_*.cpp files
   - Uses REGISTER_GANY_MODULE macro to register the module

#### Module Header File

Before using `autoany`, you need to create a module header file `reg_<ModuleName>.h` to declare the GAny module.

For example, for module name `Gx`, create `reg_Gx.h`:

```cpp
#ifndef GX_REG_GX_H
#define GX_REG_GX_H

#include "gx/gany_module_def.h"

GANY_MODULE_DEFINE(Gx);

#endif //GX_REG_GX_H
```

Purpose of this header file:
- Uses `GANY_MODULE_DEFINE` macro to declare the module
- Module name must match the name specified by `-m` parameter
- `autoany` generated `reg_<ModuleName>.cpp` will include this header file

#### Notes

1. Must create `reg_<ModuleName>.h` module header file first
2. All input files must be under the directory specified by `--base-path`
3. Source files starting with `reg_` will be automatically skipped
4. Output directory will be created automatically if it doesn't exist
5. Generated code depends on GAny library

### doc_make

`doc_make` is a documentation generator that extracts type information from GAny exported modules and generates interface documentation in multiple formats, including Markdown, EmmyLua, JSON, and JavaScript.

#### Usage

```bash
doc_make -p [work_path] -o [output_path] -t [doc_type] [module_file_a] [module_file_b]
```

#### Command Line Arguments

| Argument | Short | Description |
|----------|-------|-------------|
| `--help` | `-h` | Show help information |
| `--path=string` | `-p` | Working directory path |
| `--type=md\|lua\|js\|json\|all` | `-t` | Documentation type to generate (optional) |
| `--output=string` | `-o` | Output path |

#### Supported Documentation Types

- `md` - Markdown format documentation
- `lua` - EmmyLua annotation format documentation (for Lua IDE code hints)
- `json` - JSON format structured documentation
- `js` - JavaScript format documentation
- `all` - Generate all above formats

If `-t` parameter is not specified, Markdown format documentation is generated by default.

#### Usage Examples

##### Generate Markdown Documentation

```bash
doc_make -p ./modules -o ./docs -t md libMyModule.so
```

##### Generate EmmyLua Documentation

```bash
doc_make -p ./modules -o ./docs -t lua libMyModule.so
```

##### Generate All Format Documentation

```bash
doc_make -p ./modules -o ./docs -t all libMyModule.so libAnotherModule.so
```

##### Generate Documentation for Multiple Modules

```bash
doc_make -p ./ -o ./api-docs module1.dll module2.dll module3.dll
```

#### Output Structure

The tool organizes output documentation by class namespaces:

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

- Each namespace corresponds to a subdirectory
- Classes without namespaces are placed in the `GLOBAL` directory
- Each class generates documentation files in corresponding formats

#### Workflow

1. **Load Modules** - Load GAny modules from specified module files (.so/.dll/.dylib)
2. **Extract Type Information** - Get all registered class information through GAny reflection system
3. **Generate Documentation** - Generate documentation in corresponding format based on specified documentation type
4. **Organize Output** - Organize output files by namespace structure

#### Documentation Content

Generated documentation includes:

- Full class name and namespace
- Inheritance relationships
- Constructors
- Member properties (including type and access permissions)
- Member methods (including parameters and return types)
- Static methods
- Constant definitions
- Enum types and their values

#### Notes

1. Module files must be dynamic libraries exported through GAny system
2. Output directory will be created automatically if it doesn't exist
3. Multiple module files can be processed simultaneously
4. Generated documentation is based on GAny reflection registration information; unregistered types won't appear in documentation
5. Working directory path is used to locate relative paths of module files

## Dependencies

The project automatically fetches the following dependencies via Git:

- [gany](https://github.com/giarld/gany) - GAny type system core library
- [gx](https://github.com/giarld/gx) - GX base library

## License

[LICENSE](./LICENSE)
