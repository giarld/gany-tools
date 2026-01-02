//
// Created by Gxin on 25-4-29.
//

#include "cpp_types_info_gen.h"
#include <sstream>
#include <iostream>
#include <cctype>
#include <map>
#include <deque>
#include <regex>
#include <unordered_set>
#include <unordered_map>

#include "gx/debug.h"


static std::string trim(const std::string &str)
{
    const size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

static bool isIdentChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || (c == '_');
}

static std::string unpackMacroSignature(const std::string &sig)
{
    // 检查是否是 MACRO(xxx) 形式
    const size_t open = sig.find('(');
    const size_t close = sig.rfind(')');
    if (open != std::string::npos && close != std::string::npos && open < close) {
        const std::string prefix = sig.substr(0, open);
        const std::string macro = trim(prefix);

        // 简单白名单，防止误解包其他括号语法
        static const std::unordered_set<std::string> knownMacros = {
            "GFX_API_FUNC",
            "GX_API_FUNC"
        };

        if (knownMacros.contains(macro)) {
            return trim(sig.substr(open + 1, close - open - 1));
        }
    }
    return sig;
}

//=======================================================

TypesInfo CppTypesInfoGen::parse(const std::string &sourceCode)
{
    const auto commentBlocks = extractCommentBlocks(sourceCode);
    const auto parsedItems = parseCommentBlocks(commentBlocks);
    auto classes = assembleTypesInfo(parsedItems);
    return classes;
}

std::string CppTypesInfoGen::stripDefaultValue(const std::string &param)
{
    const size_t eq = param.find('=');
    if (eq != std::string::npos) {
        return trim(param.substr(0, eq));
    }
    return trim(param);
}

std::string CppTypesInfoGen::normalizeFunctionSignature(const std::string &signature)
{
    std::string result;
    bool inSpace = false;
    for (const char ch: signature) {
        if (ch == '\n' || ch == '\r') {
            if (!inSpace) {
                result += ' ';
                inSpace = true;
            }
        } else if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!inSpace) {
                result += ' ';
                inSpace = true;
            }
        } else {
            result += ch;
            inSpace = false;
        }
    }
    const size_t first = result.find_first_not_of(' ');
    const size_t last = result.find_last_not_of(' ');
    if (first == std::string::npos)
        return "";
    return result.substr(first, last - first + 1);
}

std::vector<FuncSigInfo> CppTypesInfoGen::generateOverloads(const FuncSigInfo &sig)
{
    std::vector<FuncSigInfo> overloads;

    const size_t paramCount = sig.argTypes.size();
    size_t lastNonDefault = 0;

    // 向后寻找第一个非默认参数
    for (size_t i = paramCount; i-- > 0;) {
        if (!sig.hasDefaultArg[i]) {
            lastNonDefault = i + 1;
            break;
        }
    }

    // 从 lastNonDefault 到 paramCount，生成重载版本
    for (size_t i = lastNonDefault; i <= paramCount; ++i) {
        FuncSigInfo alt;
        alt.name = sig.name;
        alt.retType = sig.retType;
        alt.argTypes.assign(sig.argTypes.begin(), sig.argTypes.begin() + i);
        alt.argsNames.assign(sig.argsNames.begin(), sig.argsNames.begin() + i);
        alt.hasDefaultArg.assign(sig.hasDefaultArg.begin(), sig.hasDefaultArg.begin() + i);
        overloads.push_back(std::move(alt));
    }

    return overloads;
}

std::vector<std::string> CppTypesInfoGen::extractCommentBlocks(const std::string &source)
{
    std::vector<std::string> comments;
    std::istringstream input(source);
    std::string line;
    bool inBlock = false;
    bool isDocComment = false;
    std::string current;

    auto cleanCommentLine = [](const std::string &l) -> std::string {
        std::string trimmed = trim(l);
        if (trimmed.starts_with("*")) {
            trimmed = trimmed.substr(1);
            trimmed = trim(trimmed);
        }
        return trimmed;
    };

    while (std::getline(input, line)) {
        std::string trimmed = trim(line);

        if (!inBlock) {
            for (const char ch: trimmed) {
                if (ch == '{') {
                    comments.emplace_back("@begin_scope");
                } else if (ch == '}') {
                    comments.emplace_back("@end_scope");
                }
            }

            if (trimmed.starts_with("/*")) {
                inBlock = true;
                isDocComment = true;
                current.clear();
                if (trimmed.size() > 3) {
                    std::string rest = trimmed.substr(3);
                    if (rest.find("*/") != std::string::npos) {
                        current += cleanCommentLine(rest.substr(0, rest.find("*/"))) + "\n";
                        inBlock = false;
                    } else {
                        current += cleanCommentLine(rest) + "\n";
                    }
                }
            } else if (trimmed.starts_with("///")) {
                inBlock = true;
                isDocComment = false;
                current.clear();
                current += trimmed.substr(3) + "\n";
            }
        } else {
            if (isDocComment) {
                size_t pos = trimmed.find("*/");
                if (pos != std::string::npos) {
                    current += cleanCommentLine(trimmed.substr(0, pos)) + "\n";
                    inBlock = false;
                } else {
                    current += cleanCommentLine(trimmed) + "\n";
                }
            } else {
                if (trimmed.starts_with("///")) {
                    current += trimmed.substr(3) + "\n";
                } else {
                    inBlock = false;
                    // current已经收集完所有 /// 注释行了，准备提取函数签名
                    bool needFuncSig = current.find("@construct") != std::string::npos ||
                                       current.find("@func") != std::string::npos ||
                                       current.find("@static_func") != std::string::npos ||
                                       current.find("@meta_func") != std::string::npos;

                    bool hasBeginScope = false;
                    bool hasEndScope = false;
                    if (needFuncSig) {
                        std::string funcSig;
                        funcSig += trimmed + " "; // 当前行（不是///）也属于函数签名一部分
                        if (funcSig.find('{') != std::string::npos) {
                            hasBeginScope = true;
                        }
                        if (funcSig.find('}') != std::string::npos) {
                            hasEndScope = true;
                        }

                        if (funcSig.find(';') == std::string::npos ||
                            funcSig.find('{') == std::string::npos) {
                            while (std::getline(input, line)) {
                                std::string codeLine = trim(line);
                                if (codeLine.empty())
                                    continue;
                                if (codeLine.starts_with("///") || codeLine.starts_with("/**"))
                                    break;
                                funcSig += codeLine + " ";
                                if (codeLine.find(';') != std::string::npos ||
                                    codeLine.find('{') != std::string::npos) {
                                    if (codeLine.find('{') != std::string::npos) {
                                        hasBeginScope = true;
                                    }
                                    if (codeLine.find('}') != std::string::npos) {
                                        hasEndScope = true;
                                    }
                                    break;
                                }
                            }
                        }

                        funcSig = normalizeFunctionSignature(funcSig);
                        if (!funcSig.empty()) {
                            current += "@func_sig " + funcSig + "\n";
                        }
                    } else {
                        for (const char ch: trimmed) {
                            if (ch == '{') {
                                hasBeginScope = true;
                            } else if (ch == '}') {
                                hasEndScope = true;
                            }
                        }
                    }

                    comments.push_back(current);
                    current.clear();

                    if (hasBeginScope) {
                        comments.emplace_back("@begin_scope");
                    }
                    if (hasEndScope) {
                        comments.emplace_back("@end_scope");
                    }
                }
            }

            // doc注释 /** */ 情况，注释结束后也要处理函数签名
            if (!inBlock && isDocComment && !current.empty()) {
                bool needFuncSig = current.find("@construct") != std::string::npos ||
                                   current.find("@func") != std::string::npos ||
                                   current.find("@static_func") != std::string::npos ||
                                   current.find("@meta_func") != std::string::npos;

                bool hasBeginScope = false;
                bool hasEndScope = false;
                if (needFuncSig) {
                    std::string funcSig;
                    while (std::getline(input, line)) {
                        std::string codeLine = trim(line);
                        if (codeLine.empty())
                            continue;
                        if (codeLine.starts_with("///") || codeLine.starts_with("/**"))
                            break;
                        funcSig += codeLine + " ";
                        if (codeLine.find(';') != std::string::npos ||
                            codeLine.find('{') != std::string::npos) {
                            if (codeLine.find('{') != std::string::npos) {
                                hasBeginScope = true;
                            }
                            if (codeLine.find('}') != std::string::npos) {
                                hasEndScope = true;
                            }
                            break;
                        }
                    }
                    funcSig = normalizeFunctionSignature(funcSig);
                    if (!funcSig.empty()) {
                        current += "@func_sig " + funcSig + "\n";
                    }
                }

                comments.push_back(current);
                current.clear();

                if (hasBeginScope) {
                    comments.emplace_back("@begin_scope");
                }
                if (hasEndScope) {
                    comments.emplace_back("@end_scope");
                }
            }
        }
    }

    return comments;
}

std::vector<CppTypesInfoGen::TagItem> CppTypesInfoGen::parseDocComment(const std::string &commentText)
{
    std::vector<TagItem> items;
    std::istringstream input(commentText);
    std::string line;
    TagItem currentItem;

    while (std::getline(input, line)) {
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty())
            continue;

        if (trimmedLine[0] == '@') {
            if (!currentItem.tag.empty() || !currentItem.value.empty()) {
                items.push_back(currentItem);
                currentItem = TagItem();
            }
            size_t spacePos = trimmedLine.find(' ');
            if (spacePos != std::string::npos) {
                currentItem.tag = trimmedLine.substr(1, spacePos - 1);
                currentItem.value = trimmedLine.substr(spacePos + 1);
            } else {
                currentItem.tag = trimmedLine.substr(1);
                currentItem.value = "";
            }
        } else {
            if (!currentItem.tag.empty() || !currentItem.value.empty()) {
                items.push_back(currentItem);
                currentItem = TagItem();
            }
            currentItem.tag = "";
            currentItem.value = trimmedLine;
        }
    }

    if (!currentItem.tag.empty() || !currentItem.value.empty()) {
        items.push_back(currentItem);
    }
    return items;
}

std::vector<CppTypesInfoGen::ParsedItem> CppTypesInfoGen::parseCommentBlocks(const std::vector<std::string> &commentBlocks)
{
    std::vector<ParsedItem> parsedItems;

    for (const auto &block: commentBlocks) {
        ParsedItem item;
        item.tags = parseDocComment(block);
        parsedItems.push_back(item);
    }

    return parsedItems;
}

FuncSigInfo CppTypesInfoGen::parseFunctionSignature(const std::string &signature, const std::string &className)
{
    FuncSigInfo info;
    std::string sig = normalizeFunctionSignature(signature);
    sig = unpackMacroSignature(sig);

    size_t lparen = sig.find('(');
    size_t rparen = sig.find(')');
    if (lparen == std::string::npos || rparen == std::string::npos || rparen < lparen)
        return info;

    std::string beforeParen = trim(sig.substr(0, lparen));
    std::string insideParen = sig.substr(lparen + 1, rparen - lparen - 1);

    auto [name, ret] = extractFunctionNameAndReturnType(beforeParen, className);
    info.name = name;
    info.retType = ret;

    // 处理参数
    std::vector<std::string> params;
    std::string currentParam;
    int parenLevel = 0;
    for (char c: insideParen) {
        if (c == ',' && parenLevel == 0) {
            params.push_back(trim(currentParam));
            currentParam.clear();
        } else {
            if (c == '<')
                ++parenLevel;
            if (c == '>')
                --parenLevel;
            currentParam += c;
        }
    }
    if (!currentParam.empty())
        params.push_back(trim(currentParam));

    int unnamedCount = 0;
    for (auto &p: params) {
        bool hasDefault = p.find('=') != std::string::npos;
        p = stripDefaultValue(p);
        if (p.empty())
            continue;

        size_t nameEnd = p.size();
        while (nameEnd > 0 && !isIdentChar(p[nameEnd - 1]))
            --nameEnd;
        size_t nameStart = nameEnd;
        while (nameStart > 0 && isIdentChar(p[nameStart - 1]))
            --nameStart;

        std::string varName = p.substr(nameStart, nameEnd - nameStart);
        std::string typePart = trim(p.substr(0, nameStart));

        if (varName.empty()) {
            varName = "arg" + std::to_string(unnamedCount++);
        }

        info.argTypes.push_back(typePart);
        info.argsNames.push_back(varName);
        info.hasDefaultArg.push_back(hasDefault);
    }

    return info;
}

TypesInfo CppTypesInfoGen::assembleTypesInfo(const std::vector<ParsedItem> &parsedItems)
{
    std::vector<std::shared_ptr<ClassInfo> > classes;
    std::vector<std::shared_ptr<EnumClassInfo>> enumClasses;
    std::vector<ClassInfo *> classStack;
    std::vector<std::string> scopeStack;
    std::map<std::string, PropertyInfo *> propertyMap;
    std::string pendingScopeType;

    TypesInfo typesInfo{};

    std::string globalNS;

    for (const auto &item: parsedItems) {
        std::string currentDoc;
        std::map<std::string, std::string> tagMap;
        for (const auto &tag: item.tags) {
            tagMap[tag.tag] = tag.value;
            if (tag.tag.empty() || tag.tag == "brief" || tag.tag == "note") {
                currentDoc += tag.value + "\n";
            }
        }
        if (!currentDoc.empty() && currentDoc.back() == '\n') {
            currentDoc = currentDoc.substr(0, currentDoc.size() - 1);
        }

        if (tagMap.contains("ns") && scopeStack.empty()) {
            globalNS = tagMap["ns"];
        }

        if (tagMap.contains("cpp_ns") && scopeStack.empty()) {
            typesInfo.cppNamespace = tagMap["cpp_ns"];
        }

        if (tagMap.contains("include_from")) {
            std::string v = tagMap["include_from"];
            if (!v.empty()) {
                typesInfo.includeFromSet.insert(v);
            }
            continue;
        }

        if (tagMap.contains("begin_scope")) {
            scopeStack.push_back(pendingScopeType.empty() ? "unknown" : pendingScopeType);
            pendingScopeType.clear();
            continue;
        }

        if (tagMap.contains("end_scope")) {
            if (!scopeStack.empty()) {
                if (scopeStack.back() == "class" && !classStack.empty()) {
                    classStack.pop_back();
                }
                scopeStack.pop_back();
            }
            continue;
        }

        if (tagMap.contains("using_ns")) {
            for (const auto &t: item.tags) {
                if (t.tag == "using_ns") {
                    typesInfo.usingNameSpaces.push_back(t.value);
                }
            }

            continue;
        }

        if (tagMap.contains("def_enum")) {
            auto enumInfo = std::make_shared<EnumClassInfo>();
            enumInfo->isDefEnum = true;
            enumInfo->name = tagMap["def_enum"];
            enumInfo->doc = currentDoc;
            enumInfo->ns = globalNS;
            if (tagMap.contains("ns")) {
                enumInfo->ns = tagMap["ns"];
            }
            if (tagMap.contains("cpp_name")) {
                enumInfo->cppName = tagMap["cpp_name"];
            } else {
                enumInfo->cppName = enumInfo->name;
            }

            enumClasses.push_back(enumInfo);
            continue;
        }

        if (tagMap.contains("enum") && scopeStack.empty()) {
            auto enumInfo = std::make_shared<EnumClassInfo>();
            enumInfo->name = tagMap["enum"];
            if (tagMap.contains("cpp_name")) {
                enumInfo->cppName = tagMap["cpp_name"];
            } else {
                enumInfo->cppName = enumInfo->name;
            }
            enumInfo->ns = globalNS;
            if (tagMap.contains("ns")) {
                enumInfo->ns = tagMap["ns"];
            }
            enumInfo->doc = currentDoc;
            if (tagMap.contains("cast_to"))
                enumInfo->castTo = tagMap["cast_to"];
            for (const auto &t: item.tags) {
                if (t.tag == "enum_item")
                    enumInfo->enumItems.push_back(t.value);
            }
            enumClasses.push_back(enumInfo);
            continue;
        }

        if (tagMap.contains("ref_code")) {
            typesInfo.customRefCode = currentDoc;
            continue;
        }

        if (tagMap.contains("class") || tagMap.contains("struct")) {
            pendingScopeType = "class";

            auto cls = std::make_shared<ClassInfo>();
            cls->name = tagMap.contains("class") ? tagMap["class"] : tagMap["struct"];
            cls->doc = currentDoc;

            cls->ns = globalNS;
            if (tagMap.contains("ns")) {
                cls->ns = tagMap["ns"];
            }

            if (tagMap.contains("cpp_name")) {
                cls->cppName = tagMap["cpp_name"];
            } else {
                cls->cppName = cls->name;
            }

            if (tagMap.contains("inherit")) {
                for (const auto &t: item.tags) {
                    if (t.tag == "inherit") {
                        cls->parents.push_back(t.value);
                    }
                }
            }

            if (!classStack.empty()) {
                std::string outer;
                std::string outerCpp;
                for (size_t i = 0; i < classStack.size(); ++i) {
                    if (i > 0) {
                        outer += ".";
                        outerCpp += "::";
                    }
                    outer += classStack[i]->name;
                    outerCpp += classStack[i]->cppName;
                }
                cls->outerClass = outer;
                cls->outerCppName = outerCpp;
            }

            classes.push_back(cls);
            ClassInfo *currentClass = cls.get();
            classStack.push_back(currentClass);
            propertyMap.clear();
            continue;
        }

        if (classStack.empty())
            continue;
        ClassInfo *currentClass = classStack.back();

        if (tagMap.contains("constant")) {
            auto constantInfo = std::make_shared<ConstantInfo>();
            constantInfo->name = tagMap["constant"];
            currentClass->constants.push_back(constantInfo);
        }

        if (tagMap.contains("alias")) {
            GString alias = tagMap["alias"];
            auto splits = alias.split('=');
            if (splits.size() == 2) {
                std::string newName = trim(splits[0].toStdString());
                std::string oldName = trim(splits[1].toStdString());

                currentClass->aliases[newName] = oldName;
            }
        }

        if (tagMap.contains("enum")) {
            auto enumInfo = std::make_shared<EnumInfo>();
            enumInfo->name = tagMap["enum"];
            enumInfo->doc = currentDoc;
            if (tagMap.contains("cpp_name")) {
                enumInfo->cppName = tagMap["cpp_name"];
            } else {
                enumInfo->cppName = enumInfo->name;
            }
            if (tagMap.contains("cast_to"))
                enumInfo->castTo = tagMap["cast_to"];
            for (const auto &t: item.tags) {
                if (t.tag == "enum_item")
                    enumInfo->enumItems.push_back(t.value);
            }
            currentClass->enums.push_back(enumInfo);
        }

        if (tagMap.contains("property")) {
            auto prop = std::make_shared<PropertyInfo>();
            prop->name = tagMap["property"];
            prop->doc = currentDoc;

            if (tagMap.contains("pack_again")) {
                prop->packAgain = true;
                prop->type = tagMap["pack_again"];
            }

            currentClass->properties.push_back(prop);
            propertyMap[prop->name] = prop.get();
        }

        if (tagMap.contains("default_construct")) {
            auto func = std::make_shared<FuncInfo>();
            func->overloads.push_back({});
            currentClass->constructs.push_back(func);
        }

        if (tagMap.contains("construct") || tagMap.contains("func") || tagMap.contains("static_func") || tagMap.contains("meta_func")) {
            auto func = std::make_shared<FuncInfo>();
            func->doc = currentDoc;

            if (tagMap.contains("construct"))
                func->name = tagMap["construct"];
            else if (tagMap.contains("func"))
                func->name = tagMap["func"];
            else if (tagMap.contains("static_func")) {
                func->name = tagMap["static_func"];
                func->isStatic = true;
            } else if (tagMap.contains("meta_func")) {
                func->name = tagMap["meta_func"];
                func->isMetaFunc = true;
            }

            if (tagMap.contains("func_sig")) {
                auto fullSig = parseFunctionSignature(tagMap["func_sig"], currentClass->name);
                func->overloads = generateOverloads(fullSig);
                if (func->name.empty() && !func->overloads.empty()) {
                    func->name = func->overloads.front().name;
                }
            }

            if (tagMap.contains("construct")) {
                currentClass->constructs.push_back(func);
            } else {
                {
                    FuncInfo *existing = nullptr;
                    for (auto &f: currentClass->funcs) {
                        if (f->name == func->name && f->isMetaFunc == func->isMetaFunc && f->isStatic == func->isStatic) {
                            existing = f.get();
                            break;
                        }
                    }

                    if (existing) {
                        existing->overloads.insert(
                            existing->overloads.end(),
                            func->overloads.begin(),
                            func->overloads.end()
                            );
                    } else {
                        currentClass->funcs.push_back(func);
                    }
                }

                if (tagMap.contains("property_get") || tagMap.contains("property_set")) {
                    std::string propName = tagMap.contains("property_get") ? tagMap["property_get"] : tagMap["property_set"];
                    auto it = propertyMap.find(propName);
                    if (it == propertyMap.end()) {
                        auto prop = std::make_shared<PropertyInfo>();
                        prop->name = propName;
                        currentClass->properties.push_back(prop);
                        propertyMap[propName] = prop.get();
                        it = propertyMap.find(propName);
                    }
                    if (it != propertyMap.end()) {
                        auto lastFunc = currentClass->funcs.back();
                        if (tagMap.contains("property_get")) {
                            it->second->getter = lastFunc;
                            it->second->hasGetter = true;
                        } else {
                            it->second->setter = lastFunc;
                            it->second->hasSetter = true;
                        }
                    }
                }
            }
        }
    }

    typesInfo.classInfos = {classes.begin(), classes.end()};
    typesInfo.enumClassInfos = enumClasses;

    resolveAllTypesFullQualified(typesInfo.classInfos);

    return typesInfo;
}

std::string CppTypesInfoGen::removeFunctionSpecifiers(const std::string &str)
{
    static const std::vector<std::string> specifiers = {
        "inline",
        "constexpr",
        "virtual",
        "explicit",
        "friend",
        "static",
        "__stdcall",
        "__cdecl",
        "__thiscall",
        "__fastcall",
        "noexcept",
        "mutable"
    };

    std::istringstream iss(str);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        bool isSpecifier = false;
        for (const auto &spec: specifiers) {
            if (token == spec) {
                isSpecifier = true;
                break;
            }
        }
        if (!isSpecifier) {
            tokens.push_back(token);
        }
    }

    std::ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0)
            oss << " ";
        oss << tokens[i];
    }
    return oss.str();
}

std::pair<std::string, std::string> CppTypesInfoGen::extractFunctionNameAndReturnType(const std::string &sigHead, const std::string &className)
{
    const std::string trimmed = trim(sigHead);
    std::string name;
    std::string retType;

    static const std::regex opRegex(R"(operator\s*(\[\]|==|!=|<=|>=|<=>|<|>|&&|\|\||\+|-|\*|/|%|\^|&|\||~|!|=|\(\)|\[\]|\w+(::\w+)*))");
    std::smatch match;

    if (std::regex_search(trimmed, match, opRegex)) {
        name = "operator" + match.str(1);
        retType = removeFunctionSpecifiers(trim(trimmed.substr(0, match.position(0))));
        return {name, retType};
    }

    // 非 operator 情况
    size_t end = trimmed.size();
    while (end > 0 && !isIdentChar(trimmed[end - 1]))
        --end;

    size_t start = end;
    while (start > 0 && isIdentChar(trimmed[start - 1]))
        --start;

    name = trimmed.substr(start, end - start);
    retType = removeFunctionSpecifiers(trim(trimmed.substr(0, start)));

    // 构造函数特殊处理：无返回类型
    if (name == className) {
        retType = "";
    }

    return {name, retType};
}

void CppTypesInfoGen::resolveAllTypesFullQualified(std::vector<std::shared_ptr<ClassInfo> > &allClasses)
{
    std::unordered_map<std::string, std::string> typeNameMap;

    for (const auto &cls: allClasses) {
        std::string classPath = cls->outerCppName.empty() ? cls->cppName : cls->outerCppName + "::" + cls->cppName;

        typeNameMap[cls->cppName] = classPath;
        typeNameMap[classPath] = classPath;

        for (const auto &i: cls->aliases) {
            typeNameMap[i.first] = i.second;
        }

        std::vector<std::string> outerChain;
        if (!cls->outerCppName.empty()) {
            std::istringstream iss(cls->outerCppName);
            std::string part;
            while (std::getline(iss, part, '.')) {
                outerChain.push_back(part);
            }
        }

        for (const auto &e: cls->enums) {
            std::string fullEnumPath = classPath + "::" + e->cppName;
            typeNameMap[e->cppName] = fullEnumPath;
            typeNameMap[cls->cppName + "::" + e->cppName] = fullEnumPath;
            typeNameMap[fullEnumPath] = fullEnumPath;

            if (!outerChain.empty()) {
                std::string scoped = outerChain[0];
                for (size_t i = 1; i < outerChain.size(); ++i)
                    scoped += "::" + outerChain[i];
                scoped += "::" + cls->cppName + "::" + e->cppName;
                typeNameMap[scoped] = fullEnumPath;
            }
        }

        for (const auto &other: allClasses) {
            if (other->outerCppName == classPath) {
                std::string fullSubClass = classPath + "::" + other->cppName;
                typeNameMap[other->cppName] = fullSubClass;
                typeNameMap[cls->cppName + "::" + other->cppName] = fullSubClass;
                typeNameMap[fullSubClass] = fullSubClass;
            }
        }
    }

    auto splitQualified = [](const std::string &type) -> std::pair<std::string, std::string> {
        const auto pos = type.rfind("::");
        if (pos == std::string::npos)
            return {"", type};
        return {type.substr(0, pos), type.substr(pos + 2)};
    };

    auto fixType = [&](std::string &type) {
        std::string trimmed = trim(type);
        std::string prefix, core, suffix;

        size_t start = 0;
        if (trimmed.size() >= 6 && trimmed.compare(0, 6, "const ") == 0) {
            prefix = "const ";
            start = 6;
        }

        size_t end = trimmed.find_first_of("&*", start);
        if (end == std::string::npos)
            end = trimmed.size();

        core = trim(trimmed.substr(start, end - start));
        suffix = trim(trimmed.substr(end));

        if (typeNameMap.contains(core)) {
            type = prefix + typeNameMap[core] + suffix;
            return;
        }

        auto [scope, base] = splitQualified(core);
        for (const auto &[shortName, qualified]: typeNameMap) {
            auto [fullScope, fullBase] = splitQualified(qualified);
            if (base == fullBase) {
                if (scope.empty() || scope == fullScope.substr(fullScope.rfind("::") + 1)) {
                    type = prefix + qualified + suffix;
                    break;
                }
            }
        }
    };

    auto fixSig = [&](FuncSigInfo &sig) {
        for (auto &t: sig.argTypes) {
            fixType(t);
        }
    };

    for (auto &cls: allClasses) {
        for (auto &f: cls->funcs) {
            for (auto &sig: f->overloads) {
                fixSig(sig);
            }
        }
        for (auto &f: cls->constructs) {
            for (auto &sig: f->overloads) {
                fixSig(sig);
            }
        }
    }
}
