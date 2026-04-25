//
// Created by Gxin on 25-6-5.
//

#include "gx/tools/make_js_doc.h"

#include <gx/gstring.h>

#include <cctype>
#include <unordered_set>


namespace tools
{
namespace
{
bool isIdentifierStart(const char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_' || ch == '$';
}

bool isIdentifierPart(const char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '$';
}

bool isReservedWord(const std::string &word)
{
    static const std::unordered_set<std::string> RESERVED = {
        "await", "break", "case", "catch", "class", "const", "continue",
        "debugger", "default", "delete", "do", "else", "enum", "export",
        "extends", "false", "finally", "for", "function", "if", "import",
        "in", "instanceof", "new", "null", "return", "super", "switch",
        "this", "throw", "true", "try", "typeof", "var", "void", "while",
        "with", "yield", "let", "static", "implements", "interface",
        "package", "private", "protected", "public"
    };
    return RESERVED.find(word) != RESERVED.end();
}

std::string escapeJsString(const std::string &value)
{
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (const char ch: value) {
        switch (ch) {
            case '\\':
                out.append("\\\\");
                break;
            case '"':
                out.append("\\\"");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\f':
                out.append("\\f");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\t':
                out.append("\\t");
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    out.push_back('"');
    return out;
}

std::string toJsLiteral(const GAny &value)
{
    if (value.isUndefined()) {
        return "undefined";
    }
    if (value.isUserObject()) {
        return value.toObject().toJsonString(2);
    }
    return value.toJsonString(2);
}

std::string trimString(std::string value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string stripCppTypePrefix(std::string name)
{
    name = trimString(std::move(name));
    static constexpr const char *PREFIXES[] = {
        "const ",
        "struct ",
        "class ",
        "enum "
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (const char *prefix: PREFIXES) {
            const std::string prefixText = prefix;
            if (name.rfind(prefixText, 0) == 0) {
                name = trimString(name.substr(prefixText.size()));
                changed = true;
            }
        }
    }
    return name;
}

std::string toJsIdentifier(std::string name, const std::string &templatePrefix = "Template_")
{
    name = stripCppTypePrefix(std::move(name));
    const bool isTemplateType = name.find('<') != std::string::npos;
    if (name.empty()) {
        return "_";
    }
    std::string normalized;
    normalized.reserve(name.size());
    bool lastWasSeparator = false;
    for (const char ch: name) {
        if (isIdentifierPart(ch)) {
            normalized.push_back(ch);
            lastWasSeparator = false;
        } else if (!lastWasSeparator) {
            normalized.push_back('_');
            lastWasSeparator = true;
        }
    }

    while (!normalized.empty() && normalized.back() == '_') {
        normalized.pop_back();
    }
    if (normalized.empty()) {
        normalized = "_";
    }
    if (isTemplateType) {
        normalized.insert(0, templatePrefix);
    }
    if (!isIdentifierStart(normalized[0])) {
        normalized.insert(normalized.begin(), '_');
    }
    if (isReservedWord(normalized)) {
        normalized.append("_");
    }
    return normalized;
}

std::string toJsParameterIdentifier(std::string name)
{
    if (name.empty()) {
        return "_";
    }
    for (char &ch: name) {
        if (!isIdentifierPart(ch)) {
            ch = '_';
        }
    }
    if (!isIdentifierStart(name[0])) {
        name.insert(name.begin(), '_');
    }
    if (isReservedWord(name)) {
        name.append("_");
    }
    return name;
}
}

std::string MakeJsDoc::makeClassDoc(const GAnyClass &clazz)
{
    GAny dumpObj = clazz.dump();
    if (!dumpObj.isObject()) {
        return "";
    }

    std::string className = dumpObj["class"].toString();
    const std::string &fullName = className;
    const std::string jsClassName = transformClassName(fullName);

    std::stringstream os;

    os << "/**\n";
    if (dumpObj.contains("doc") && dumpObj["doc"] != "") {
        os << transformDocString(dumpObj["doc"].toString(), " * ") << "\n";
    }
    os << " * @class " << fullName << "\n"
        << " * @namespace " << dumpObj["nameSpace"].toString() << "\n"
        << " **/\n"
        << "class " << jsClassName;

    if (dumpObj.contains("parents") && dumpObj["parents"].size() > 0) {
        os << " extends " << transformBaseClassName(dumpObj["parents"][0].toString());
        if (dumpObj["parents"].size() > 1) {
            os << " /*";
            for (size_t i = 1; i < dumpObj["parents"].size(); i++) {
                os << " also extends " << transformBaseClassName(dumpObj["parents"][i].toString());
            }
            os << " */";
        }
        os << " {";
    } else {
        os  << " extends GAnyClass {";
    }

    bool first = true;
    //
    {
        for (const auto it = dumpObj["constants"].iterator(); it.hasNext();) {
            auto p = it.next().second;
            if (!p.isObject()) {
                continue;
            }
            std::string pName = p["name"].toString();
            GAny v = p["value"];

            os << (first ? "\n" : "\n\n");
            first = false;

            os << "    /**\n";
            os << "     * @static\n";
            os << "     * @readonly\n";
            os << "     **/\n";
            os << "    static " << transformMemberName(pName) << " = " << toJsLiteral(v) << ";";
        }

        for (const auto it = dumpObj["properties"].iterator(); it.hasNext();) {
            auto p = it.next().second;
            if (!p.isObject()) {
                continue;
            }

            os << (first ? "\n" : "\n\n");
            first = false;

            std::string type = transformType(p["type"].toString());
            std::string pName = p["name"].toString();
            if (!type.empty()) {
                os << "    /** @property {" << type << "} " << pName << " **/\n";
            }

            os << "    " << transformMemberName(pName) << ";";
        }

        for (auto it = dumpObj["methods"].iterator(); it.hasNext();) {
            const auto m = it.next().second;
            if (!m.isObject()) {
                continue;
            }

            if (!m.contains("overloads")) {
                continue;
            }
            GString fullMethodName = m["name"].toString();
            auto nameSplits = fullMethodName.split(".");

            std::string methodName;
            if (nameSplits.size() == 1) {
                methodName = nameSplits[0].toStdString();
            } else {
                methodName = nameSplits[1].toStdString();
            }

            methodName = transformMethodName(methodName);

            if (methodName.empty()) {
                continue;
            }

            bool isNewMethod = methodName == "new";
            bool isStatic = isNewMethod || m["isStatic"].toBool();
            const std::string jsMethodName = transformMemberName(methodName);

            auto overloads = m["overloads"];

            for (auto it2 = overloads.iterator(); it2.hasNext();) {
                const auto &ovi = it2.next().second;
                if (!ovi.isObject()) {
                    continue;
                }

                os << (first ? "\n" : "\n\n");
                first = false;


                const auto &args = ovi["args"];

                os << "    /**\n";
                if (ovi.contains("doc") && ovi["doc"] != "") {
                    os << transformDocString(ovi["doc"].toString(), "     * ") << "\n";
                }

                if (isStatic) {
                    os << "     * @static\n";
                }

                for (auto argIt = args.iterator(); argIt.hasNext();) {
                    const auto &arg = argIt.next().second;
                    if (!arg.isObject()) {
                        break;
                    }

                    std::string key = arg["key"].toString();
                    std::string type = arg["type"].toString();
                    if (key == "self") {
                        continue;
                    }

                    if (key == "...") {
                        key = "args";
                        type = "...any";
                    }

                    type = transformType(type);
                    os << "     * @param {" << type << "} " << key << "\n";
                }

                std::string returnValue;
                if (isNewMethod) {
                    returnValue = className;
                } else if (ovi.contains("return")) {
                    returnValue = ovi["return"].toString();
                    returnValue = transformType(returnValue);
                }
                if (!returnValue.empty() && returnValue != "undefined") {
                    os << "     * @returns {" << returnValue << "}\n";
                }

                os << "     **/\n";

                os << "    ";
                if (isStatic) {
                    os << "static ";
                }
                os << jsMethodName << "(";

                int32_t index = 0;
                for (auto argIt = args.iterator(); argIt.hasNext();) {
                    const auto &arg = argIt.next().second;
                    if (!arg.isObject()) {
                        break;
                    }

                    std::string key = arg["key"].toString();
                    if (key == "self") {
                        continue;
                    }
                    if (index > 0) {
                        os << ", ";
                    }

                    if (key == "...") {
                        os << "...args";
                        index++;
                        continue;
                    }

                    os << toJsParameterIdentifier(key);
                    index++;
                }
                os << ") {}";
            }
        }
    }
    os << "\n}\n";

    return os.str();
}

std::string MakeJsDoc::makeFunctionDoc(const GAnyFunction &func)
{
    std::stringstream os;
    GAny dumpObj = func.dump();
    if (!dumpObj.isObject() || !dumpObj.contains("overloads")) {
        return "";
    }

    std::string functionName = dumpObj["name"].toString();
    functionName = toJsIdentifier(transformMethodName(functionName));
    if (functionName.empty()) {
        return "";
    }

    auto overloads = dumpObj["overloads"];
    for (auto it = overloads.iterator(); it.hasNext();) {
        const auto &ovi = it.next().second;
        if (!ovi.isObject()) {
            continue;
        }

        const auto &args = ovi["args"];

        os << "/**\n";
        if (ovi.contains("doc") && ovi["doc"] != "") {
            os << transformDocString(ovi["doc"].toString(), " * ") << "\n";
        }

        for (auto argIt = args.iterator(); argIt.hasNext();) {
            const auto &arg = argIt.next().second;
            if (!arg.isObject()) {
                break;
            }

            std::string key = arg["key"].toString();
            std::string type = arg["type"].toString();
            if (key == "...") {
                key = "args";
                type = "...any";
            }
            type = transformType(type);
            os << " * @param {" << type << "} " << key << "\n";
        }

        if (ovi.contains("return")) {
            std::string returnValue = transformType(ovi["return"].toString());
            if (!returnValue.empty() && returnValue != "undefined") {
                os << " * @returns {" << returnValue << "}\n";
            }
        }

        os << " **/\n"
            << "function " << functionName << "(";

        int32_t index = 0;
        for (auto argIt = args.iterator(); argIt.hasNext();) {
            const auto &arg = argIt.next().second;
            if (!arg.isObject()) {
                break;
            }
            std::string key = arg["key"].toString();
            if (index > 0) {
                os << ", ";
            }
            if (key == "...") {
                os << "...args";
                index++;
                continue;
            }
            os << toJsParameterIdentifier(key);
            index++;
        }
        os << ") {}\n";
    }

    return os.str();
}

std::string MakeJsDoc::transformClassName(const std::string &name)
{
    return toJsIdentifier(name);
}

std::string MakeJsDoc::transformBaseClassName(const std::string &name)
{
    return toJsIdentifier(name, "TemplateBase_");
}

std::string MakeJsDoc::transformMemberName(const std::string &name)
{
    if (name.empty()) {
        return "_";
    }

    if (!isIdentifierStart(name[0]) || isReservedWord(name)) {
        return "[" + escapeJsString(name) + "]";
    }
    for (size_t i = 1; i < name.size(); ++i) {
        if (!isIdentifierPart(name[i])) {
            return "[" + escapeJsString(name) + "]";
        }
    }
    return name;
}

std::string MakeJsDoc::transformType(const std::string &type)
{
    GString sType = type;
    sType = sType.toLower();
    sType = sType.replace("const ", "").replace("&", "").replace("*", "");
    const std::string normalizedType = trimString(sType.toStdString());
    sType = normalizedType;

    if (sType.indexOf("function") >= 0) {
        return "function|GAnyUserObject";
    }
    if (sType.indexOf("std") >= 0) {
        if (sType.indexOf("vector") >= 0) {
            return "array|GAnyUserObject";
        }
        if (sType.indexOf("list") >= 0) {
            return "array|GAnyUserObject";
        }
        if (sType.indexOf("map") >= 0) {
            return "object|GAnyUserObject";
        }
    }

    if (sType.indexOf(" ") >= 0) {
        return "";
    }
    if (sType == "gany" || sType == "any") {
        return "any";
    }
    if (sType == "void" || sType == "undefined") {
        return "undefined";
    }

    if (sType == "int" || sType == "int32" || sType == "uint32"
        || sType == "int64" || sType == "uint64"
        || sType == "int8" || sType == "uint8"
        || sType == "int16" || sType == "uint16"
        || sType == "float" || sType == "double") {
        return "number";
    }
    if (sType == "bool") {
        return "boolean";
    }
    if (sType == "null") {
        return "null";
    }
    if (sType == "nil") {
        return "null";
    }

    return type;
}

std::string MakeJsDoc::transformMethodName(const std::string &name)
{
    const GString sName = name;

    // Hidden operators
    if (sName.startWith("__")) {
        if (sName == "__init") {
            return "new";
        }
        return "";
    }

    return name;
}

std::string MakeJsDoc::transformDocString(const std::string &doc, const std::string &commentPrefix)
{
    std::stringstream ss(doc);

    std::string line;
    std::string newDoc;
    int32_t lineIndex = 0;
    while (std::getline(ss, line, '\n')) {
        if (lineIndex > 0) {
            newDoc.append("\n");
        }
        newDoc.append(commentPrefix);
        newDoc.append(line);
        ++lineIndex;
    }
    return newDoc;
}
} // tools
