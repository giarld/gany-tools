//
// Created by Gxin on 25-4-29.
//

#ifndef CPP_TYPES_INFO_GEN_H
#define CPP_TYPES_INFO_GEN_H

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>


struct FuncSigInfo
{
    std::string name;
    std::vector<std::string> argTypes;
    std::vector<std::string> argsNames;
    std::vector<bool> hasDefaultArg;
    std::string retType;
};

struct FuncInfo
{
    std::string name;
    std::vector<FuncSigInfo> overloads;
    std::string doc;
    bool isMetaFunc = false;
    bool isStatic = false;
};

struct PropertyInfo
{
    std::string name;
    std::shared_ptr<FuncInfo> getter;
    std::shared_ptr<FuncInfo> setter;
    bool hasGetter = false;
    bool hasSetter = false;
    bool packAgain = false;
    std::string type;
    std::string doc;
};

struct EnumInfo
{
    std::string name;
    std::string cppName;
    std::string castTo;
    std::vector<std::string> enumItems;
    std::string doc;
};

struct EnumClassInfo
{
    std::string name;
    std::string cppName;
    std::string ns;
    std::string castTo;
    std::vector<std::string> enumItems;
    std::string doc;
    bool isDefEnum = false;
};

struct ConstantInfo
{
    std::string name;
};

struct ClassInfo
{
    std::string name;
    std::string cppName;
    std::string ns;
    std::string outerClass;
    std::string outerCppName;
    std::string doc;
    std::vector<std::string> parents;
    std::vector<std::shared_ptr<FuncInfo> > constructs;
    std::vector<std::shared_ptr<FuncInfo> > funcs;
    std::vector<std::shared_ptr<PropertyInfo> > properties;
    std::vector<std::shared_ptr<EnumInfo> > enums;
    std::vector<std::shared_ptr<ConstantInfo> > constants;
    std::unordered_map<std::string, std::string> aliases;
};

struct TypesInfo
{
    std::string cppNamespace;
    std::vector<std::shared_ptr<ClassInfo> > classInfos;
    std::vector<std::shared_ptr<EnumClassInfo> > enumClassInfos;
    std::vector<std::string> usingNameSpaces;
    std::set<std::string> includeFromSet; // 提前声明从哪些头文件引入
    std::string customRefCode;
};


class CppTypesInfoGen
{
public:
    static TypesInfo parse(const std::string &sourceCode);

private:
    struct TagItem
    {
        std::string tag;
        std::string value;
    };

    struct ParsedItem
    {
        std::vector<TagItem> tags;
    };

    static std::string stripDefaultValue(const std::string &param);

    static std::string normalizeFunctionSignature(const std::string &signature);

    static std::vector<FuncSigInfo> generateOverloads(const FuncSigInfo &sig);

    static std::vector<std::string> extractCommentBlocks(const std::string &source);

    static std::vector<TagItem> parseDocComment(const std::string &commentText);

    static std::vector<ParsedItem> parseCommentBlocks(const std::vector<std::string> &commentBlocks);

    static FuncSigInfo parseFunctionSignature(const std::string &signature, const std::string &className);

    static TypesInfo assembleTypesInfo(const std::vector<ParsedItem> &parsedItems);

    static std::string removeFunctionSpecifiers(const std::string &str);

    static std::pair<std::string, std::string> extractFunctionNameAndReturnType(const std::string &sigHead, const std::string &className);

    static void resolveAllTypesFullQualified(std::vector<std::shared_ptr<ClassInfo> > &allClasses);
};

#endif //CPP_TYPES_INFO_GEN_H
