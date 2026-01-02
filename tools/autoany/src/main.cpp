//
// Created by Gxin on 25-4-29.
//

#include "to_any_gen.h"

#define USE_GANY_CORE
#include <gx/gany.h>

#include <gx/debug.h>
#include <gx/gfile.h>

#include <getopt/getopt.h>


static std::string sOutput;
static std::string sBasePath;
static std::string sIncludePrefix;
static std::string sModuleName;


struct FileReflecInfo
{
    std::string refFileName;
    std::string refFuncName;
};


static const char *USAGE = R"TXT(Usage:
APP_NAME [options] -m "Gx" -b "include/gx" -p "gx/" -o "./gx/toany/" gx/include/a.h gx/include/b.h

Automatically parse source code files and generate reflection code for classes, structures, and enumerations.

Options:
    --help, -h
        Print this message.
    --module-name=string, -m string
        Module name generated.
    --base-path=string, -b string
        Base path, the starting path of the header file that needs to be parsed.
    --include-prefix=string, -p string
        When generating code, include the prefix of the source file.
    --output=string, -o string
        Output path.

Doc Tags:
    @using_ns [namespace]       Indicates the need to using a namespace.
    @ns [name]                  Represents the namespace after class/enum_class reflection.
    @cpp_ns [name]              C++ namespace.
    @include_from [path]        Used to indicate which header files to import from in advance declaration.
    @class [name]               Indicate the beginning of the class, that is, the current class needs to be reflected.
    @struct [name]              Indicate the beginning of the structure, that is, the current structure needs to be reflected.
    @inherit [name]             Indicate which base class the class inherits from, and there can be multiple.
    @enum [name]                Represents enumeration.
    @enum_item [name]           There can be multiple items representing enumeration, one per line.
    @cast_to [type]             The enumeration value needs to be type converted to the specified type.
    @construct                  Represents a constructor.
    @default_construct          Indicates the existence of a default constructor, which can be directly commented in the blank space within the class without the need for a function declaration below.
    @func [name]                Represents a member function (Name optional).
    @static_func [name]         Represents a static member function.
    @meta_func [name]           Representing a metafunction, corresponding to the enumeration value of the MetaFunction, such as: ToString.
    @property_get [name]        The representation function is a get function for a member property.
    @property_set [name]        The representation function is a set function of a member property.
    @property [name]            Representing member property.
    @pack_again [type]          Repackaging the property will be done using REF_PROPERTY_RW.
    @constant [name]            Represents a constant.
    @cpp_name [name]            List the CPP type names of the enumeration class.
    @def_enum                   It is a label specific to DEF_ENUM_N and DEF_ENUM_FLAGS_N.
    @ref_code                   Custom reflection code blocks are written after @ ref_comde line breaks, supporting multiple lines, and will be inserted into the reflection function as is.
    @alias                      Ex: @alias NewName = OldName.
)TXT";

static void printUsage(const char *name)
{
    const std::string execName(GFile(name).fileName());
    const std::string from("APP_NAME");
    std::string usage(USAGE);
    for (size_t pos = usage.find(from); pos != std::string::npos; pos = usage.find(from, pos)) {
        usage.replace(pos, from.length(), execName);
    }
    puts(usage.c_str());
}

static int handleArguments(int argc, char *argv[])
{
    constexpr static const char *OPTSTR = "hm:b:p:o:";
    static const option OPTIONS[] = {
        {"help", no_argument, nullptr, 'h'},
        {"module-name", required_argument, nullptr, 'm'},
        {"base-path", required_argument, nullptr, 'b'},
        {"include-prefix", required_argument, nullptr, 'p'},
        {"output", required_argument, nullptr, 'o'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int optionIndex = 0;

    while ((opt = gp_getopt_long(argc, argv, OPTSTR, OPTIONS, &optionIndex)) >= 0) {
        std::string arg(optarg ? optarg : "");
        switch (opt) {
            default:
            case 'h': {
                printUsage(argv[0]);
                exit(0);
            }
            case 'm': {
                sModuleName = arg;
            }
            break;
            case 'b': {
                sBasePath = arg;
            }
            break;
            case 'p': {
                sIncludePrefix = arg;
            }
            break;
            case 'o': {
                sOutput = arg;
            }
            break;
        }
    }

    return optind;
}

int32_t parseFile(GFile &file, FileReflecInfo &info)
{
    if (!file.open(GFile::ReadOnly)) {
        return -1;
    }

    std::string source = file.readAll().toStdString();
    file.close();

    const TypesInfo typesInfo = CppTypesInfoGen::parse(source);

    const GString srcFilePath = file.absoluteFilePath();
    const GString srcFileNameWE = file.fileNameWithoutExtension();
    const GString srcShortPath = srcFilePath.substring(sBasePath.size());

    info.refFileName = "ref_" + srcFileNameWE.toStdString() + ".cpp";
    info.refFuncName = "ref_" + srcFileNameWE.toStdString();

    std::stringstream refCode;
    refCode << "#include <gx/gany.h>\n";
    refCode << "#include <" << sIncludePrefix << srcShortPath << ">\n";
    if (!typesInfo.includeFromSet.empty()) {
        for (const auto &i : typesInfo.includeFromSet) {
            refCode << "#include \"" << i << "\"\n";
        }
    }
    refCode << "\n";

    if (!typesInfo.cppNamespace.empty()) {
        auto nss = GString(typesInfo.cppNamespace).split("::");
        for (const auto &i : nss) {
            refCode << "using namespace " << i << ";\n";
        }
    }

    for (const auto &uns: typesInfo.usingNameSpaces) {
        if (typesInfo.cppNamespace != uns) {
            refCode << "using namespace " << uns << ";\n";
        }
    }

    refCode << "using namespace gany;\n";

    refCode << "\nvoid " << info.refFuncName << "()\n";
    refCode << "{";

    for (const auto &enumInfo: typesInfo.enumClassInfos) {
        std::string code = ToAnyGen::genReflecEnumClassCode(*enumInfo);

        refCode << "\n";

        std::string line;
        std::istringstream input(code);
        while (std::getline(input, line)) {
            if (line.empty()) {
                refCode << "\n";
                continue;
            }
            refCode << "    " << line << "\n";
        }
    }

    for (const auto &classInfo: typesInfo.classInfos) {
        std::string code = ToAnyGen::genReflecClassCode(*classInfo);

        refCode << "\n";

        std::string line;
        std::istringstream input(code);
        while (std::getline(input, line)) {
            if (line.empty()) {
                refCode << "\n";
                continue;
            }
            refCode << "    " << line << "\n";
        }
    }

    if (!typesInfo.customRefCode.empty()) {
        refCode << "\n";
        std::string line;
        std::istringstream input(typesInfo.customRefCode);
        while (std::getline(input, line)) {
            if (line.empty()) {
                refCode << "\n";
                continue;
            }
            refCode << "    " << line << "\n";
        }
    }

    refCode << "}\n";

    GFile outputFile(GFile(sOutput), info.refFileName);
    if (outputFile.open(GFile::WriteOnly)) {
        outputFile.write(refCode.str());
        outputFile.close();
    }

    return 1;
}

int main(int argc, char *argv[])
{
    initGAnyCore();

    const int optionIndex = handleArguments(argc, argv);
    const int numArgs = argc - optionIndex;
    if (numArgs < 1) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (sOutput.empty()) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (sModuleName.empty()) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    //
    const GFile baseDir(sBasePath);
    if (!baseDir.isDirectory()) {
        LogE("The base path does not exist: {}", sBasePath);
        return EXIT_FAILURE;
    }
    sBasePath = baseDir.absoluteFilePath() + "/";

    //
    if (!sIncludePrefix.empty()) {
        GString tempIncludePrefix = sIncludePrefix;
        tempIncludePrefix = tempIncludePrefix.replace("\\", "/");
        if (tempIncludePrefix.startWith("/")) {
            tempIncludePrefix = tempIncludePrefix.substring(1);
        }
        if (!tempIncludePrefix.endWith("/")) {
            tempIncludePrefix.append("/");
        }
        sIncludePrefix = tempIncludePrefix.toStdString();
    }

    //
    const GFile outputDir(sOutput);
    if (!outputDir.exists()) {
        outputDir.mkdirs();
    }
    sOutput = outputDir.absoluteFilePath() + "/";

    //
    std::vector<GFile> inputFileLists;
    for (int argIndex = optionIndex; argIndex < argc; ++argIndex) {
        GFile f(argv[argIndex]);
        if (!f.exists()) {
            continue;
        }
        if (!f.absoluteFilePath().starts_with(sBasePath)) {
            LogE("The file is not under the base path: {}", f.filePath());
            continue;
        }
        if (GString(f.fileName()).startWith("reg_")) {
            continue;
        }

        inputFileLists.push_back(f);
    }

    std::vector<FileReflecInfo> fileReflecInfos;
    for (GFile &file: inputFileLists) {
        FileReflecInfo info{};
        const int32_t ret = parseFile(file, info);
        if (ret < 0) {
            LogE("Failed to generate reflection code, source file: {}", file.absoluteFilePath());
            return EXIT_FAILURE;
        }
        if (ret == 1) {
            fileReflecInfos.push_back(info);
        }
    }

    // 生成模块源文件
    if (!fileReflecInfos.empty()) {
        std::stringstream code;
        code << "#include \"" << sIncludePrefix << "reg_" << sModuleName << ".h" << "\"\n";
        code << "#include <gx/gany.h>\n\n";
        for (const auto &refInfo: fileReflecInfos) {
            code << "extern void " << refInfo.refFuncName << "();\n";
        }
        code << "\n";

        code << "REGISTER_GANY_MODULE(" << sModuleName << ")\n";
        code << "{\n";
        for (const auto &refInfo: fileReflecInfos) {
            code << "    " << refInfo.refFuncName << "();\n";
        }
        code << "}\n";

        std::string headFileName = "reg_" + sModuleName + ".cpp";
        GFile moduleHeadFile(outputDir, headFileName);
        if (moduleHeadFile.open(GFile::WriteOnly)) {
            moduleHeadFile.write(code.str());
            moduleHeadFile.close();
        }
    }

    return EXIT_SUCCESS;
}
