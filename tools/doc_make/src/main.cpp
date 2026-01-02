//
// Created by Gxin on 2022/6/20.
//

#define USE_GANY_CORE
#include <gx/gany.h>

#include <gx/gfile.h>
#include <gx/debug.h>

#include <gx/reg_gx.h>

#include <gx/tools/reg_doc_tool.h>
#include <gx/tools/make_markdown.h>
#include <gx/tools/make_emmy_lua_doc.h>
#include <gx/tools/make_json_doc.h>
#include <gx/tools/make_js_doc.h>

#include <getopt/getopt.h>

#include <unordered_set>


using namespace tools;

std::string sPath = "./";
std::string sOutputPath = "./doc";

std::unique_ptr<MakeMarkdown> sMakeMarkdown;

std::unique_ptr<MakeEmmyLuaDoc> sMakeEmmyLuaDoc;

std::unique_ptr<MakeJsonDoc> sMakeJsonDoc;

std::unique_ptr<MakeJsDoc> sMakeJsDoc;

std::vector<std::shared_ptr<GAnyClass> > getAllClasses()
{
    if (!pfnGanyGetEnv) {
        return {};
    }

    GAny envObj;
    pfnGanyGetEnv(&envObj);
    if (!envObj.isObject()) {
        return {};
    }

    GAny classDB = (*envObj.as<GAnyObject>())["__CLASS_DB"];
    if (!classDB.isUserObject()) {
        return {};
    }

    return classDB.call("getAllClasses").castAs<std::vector<std::shared_ptr<GAnyClass> > >();
}

bool makeDoc(GFile &dir, const std::string &fileName, const GAnyClass &clazz, IDocMake *makeDoc)
{
    if (!dir.exists()) {
        if (!dir.mkdirs()) {
            std::cerr << "mkdir failed: " << dir.absoluteFilePath() << std::endl;
            return false;
        }
    }
    if (!dir.isDirectory()) {
        std::cerr << "path is not a directory: " << dir.filePath() << std::endl;
        return false;
    }

    std::cout << "Make " << makeDoc->getDocType() << " doc: ";
    if (!clazz.getNameSpace().empty()) {
        std::cout << clazz.getNameSpace() << "::";
    }
    std::cout << clazz.getName() << std::endl;

    std::string dump = makeDoc->makeClassDoc(clazz);

    GFile file(dir, fileName);

    if (file.open(GFile::WriteOnly)) {
        file.write(dump.c_str(), (int32_t) dump.size());
        file.close();
    }

    return true;
}

bool foreachClass(GFile &dir)
{
    std::vector<std::shared_ptr<GAnyClass>> classes = getAllClasses();
    for (const auto &clazz : classes) {
        if (!clazz) {
            continue;
        }

        std::string ns = clazz->getNameSpace();
        std::string name = clazz->getName();

        if (ns.empty()) {
            ns = "GLOBAL";
        }

        GFile nsDir(dir, ns);
        if (!nsDir.exists()) {
            nsDir.mkdirs();
        }

        if (sMakeMarkdown) {
            if (!makeDoc(nsDir, name + ".md", *clazz, sMakeMarkdown.get())) {
                return false;
            }
        }
        if (sMakeEmmyLuaDoc) {
            if (!makeDoc(nsDir, name + ".lua", *clazz, sMakeEmmyLuaDoc.get())) {
                return false;
            }
        }
        if (sMakeJsonDoc) {
            if (!makeDoc(nsDir, name + ".json", *clazz, sMakeJsonDoc.get())) {
                return false;
            }
        }
        if (sMakeJsDoc) {
            if (!makeDoc(nsDir, name + ".js", *clazz, sMakeJsDoc.get())) {
                return false;
            }
        }
    }

    return true;
}

void printUsage(const char *program)
{
    fprintf(stdout, R"TXT(Usage:
%s -p [work_path] -o [output_path] [module_file_a] [module_file_b]

Generate interface documents of modules exported by GAny

Options:
    --path=string, -p string
        Working directory path
    --doc-type=md|lua|json|all, -t md|lua|js|json|all
        生成的文档类型.
        md: Markdown
        lua: EmmyLua
    --output=string, -o string
        Output Path
)TXT",
            program);
}

static int handleArguments(int argc, char *argv[])
{
    constexpr const char *OPT_STR = "hp:t:o:";

    const static option OPTIONS[] = {
        {"help", no_argument, nullptr, 'h'},
        {"path", required_argument, nullptr, 'p'},
        {"type", required_argument, nullptr, 't'},
        {"output", required_argument, nullptr, 'o'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    int optIndex;

    std::vector<std::string> plugins;

    while ((opt = gp_getopt_long(argc, argv, OPT_STR, OPTIONS, &optIndex)) != -1) {
        std::string arg(optarg ? optarg : "");
        switch (opt) {
            default:
            case 'h':
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'p':
                sPath = arg;
                break;
            case 't': {
                if (arg == "md" && !sMakeMarkdown) {
                    sMakeMarkdown = std::make_unique<MakeMarkdown>();
                }
                if (arg == "lua" && !sMakeEmmyLuaDoc) {
                    sMakeEmmyLuaDoc = std::make_unique<MakeEmmyLuaDoc>();
                }
                if (arg == "json" && !sMakeJsonDoc) {
                    sMakeJsonDoc = std::make_unique<MakeJsonDoc>();
                }
                if (arg == "js" && !sMakeJsDoc) {
                    sMakeJsDoc = std::make_unique<MakeJsDoc>();
                }
                if (arg == "all") {
                    if (!sMakeMarkdown) {
                        sMakeMarkdown = std::make_unique<MakeMarkdown>();
                    }
                    if (!sMakeEmmyLuaDoc) {
                        sMakeEmmyLuaDoc = std::make_unique<MakeEmmyLuaDoc>();
                    }
                    if (!sMakeJsonDoc) {
                        sMakeJsonDoc = std::make_unique<MakeJsonDoc>();
                    }
                    if (!sMakeJsDoc) {
                        sMakeJsDoc = std::make_unique<MakeJsDoc>();
                    }
                }
            }
            break;
            case 'o':
                sOutputPath = arg;
                break;
        }
    }

    return optind;
}

int main(int argc, char *argv[])
{
    initGAnyCore();

    GANY_LOAD_MODULE(DocTools);

    const int optionIndex = handleArguments(argc, argv);
    const int numArgs = argc - optionIndex;
    if (numArgs < 1) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!sMakeEmmyLuaDoc && !sMakeMarkdown && !sMakeJsonDoc && !sMakeJsDoc) {
        sMakeMarkdown = std::make_unique<MakeMarkdown>();
    }

    std::vector<std::string> plugins;

    for (int argIndex = optionIndex; argIndex < argc; ++argIndex) {
        plugins.push_back(argv[argIndex]);
    }

    /// ======================================
    GANY_LOAD_MODULE(Gx);

    for (auto &p: plugins) {
        GFile dir(sPath);
        GFile f(dir, p);
        GAny::Load(f.filePath());
    }

    GFile dir(sOutputPath);
    if (!dir.exists()) {
        dir.mkdirs();
    }
    if (!dir.exists()) {
        fprintf(stderr, "Output directory creation failed.\n");
        exit(EXIT_FAILURE);
    }

    if (!foreachClass(dir)) {
        fprintf(stderr, "foreachClass failed.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
