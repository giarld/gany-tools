//
// Created by Gxin on 24-5-17.
//

#include "gx/tools/make_emmy_lua_doc.h"

#include <gx/gstring.h>


namespace tools
{
bool isLuaControlFlowKeyword(const std::string &keyword)
{
    return keyword == "and" ||
           keyword == "break" ||
           keyword == "do" ||
           keyword == "else" ||
           keyword == "elseif" ||
           keyword == "end" ||
           keyword == "false" ||
           keyword == "for" ||
           keyword == "function" ||
           keyword == "goto" ||
           keyword == "if" ||
           keyword == "in" ||
           keyword == "local" ||
           keyword == "nil" ||
           keyword == "not" ||
           keyword == "or" ||
           keyword == "repeat" ||
           keyword == "return" ||
           keyword == "then" ||
           keyword == "true" ||
           keyword == "until" ||
           keyword == "while";
}

std::string jsonObjToLuaTable(const GAny &jsonObj)
{
    auto LuaTable = GAny::Import("L.LuaTable");
    return LuaTable.call("fromGAnyObject", jsonObj).toString();
}

std::string MakeEmmyLuaDoc::makeClassDoc(const GAnyClass &clazz)
{
    GAny dumpObj = clazz.dump();
    if (!dumpObj.isObject()) {
        return "";
    }

    std::string name = dumpObj["class"].toString();
    const std::string &fullName = name;
    std::string parent;
    if (dumpObj["parents"].isArray() && dumpObj["parents"].size() > 0) {
        parent = dumpObj["parents"][0].toString();
    }

    std::stringstream os;

    if (dumpObj.contains("doc") && dumpObj["doc"] != "") {
        os << transformDocString(dumpObj["doc"].toString(), "---") << "\n";
    }
    os << "---@class " << fullName << (parent.empty() ? "" : " : " + parent) << "\n"
        << "---@namespace " << dumpObj["nameSpace"].toString() << "\n"
        << fullName << " = {";

    if (dumpObj["properties"].size() > 0 || dumpObj["constants"].size() > 0) {
        os << "\n";
        int32_t index = 0;
        for (auto it = dumpObj["properties"].iterator(); it.hasNext();) {
            auto p = it.next().second;
            if (!p.isObject()) {
                continue;
            }
            if (index > 0) {
                os << ",\n";
            }
            std::string type = transformType(p["type"].toString());
            if (!type.empty()) {
                os << "    ---@type " << type << "\n";
            }
            os << "    ";
            std::string pName = p["name"].toString();
            if (isLuaControlFlowKeyword(pName)) {
                os << "[\"" << pName << "\"]";
            } else {
                os << pName;
            }
            os << " = {}";

            index++;
        }
        for (auto it = dumpObj["constants"].iterator(); it.hasNext();) {
            auto c = it.next().second;
            if (!c.isObject()) {
                continue;
            }
            if (index > 0) {
                os << ",\n";
            }
            os << "    ";
            std::string pName = c["name"].toString();
            if (isLuaControlFlowKeyword(pName)) {
                os << "[\"" << pName << "\"]";
            } else {
                os << pName;
            }

            if (c["value"].isObject() || c["value"].isArray() || c["value"].isUserObject()) {
                GAny obj = c["value"].toObject();
                os << " = " << jsonObjToLuaTable(obj);
            } else {
                os << " = " << c["value"];
            }

            index++;
        }
        os << "\n";
    }
    os << "};\n";

    if (dumpObj.contains("methods") && dumpObj["methods"].isArray()) {
        const auto &methods = dumpObj["methods"];
        for (auto it = methods.iterator(); it.hasNext();) {
            GAny method = it.next().second;
            makeFuncOrMethodDoc(os, method);
        }
    }

    return os.str();
}

std::string MakeEmmyLuaDoc::makeFunctionDoc(const GAnyFunction &func)
{
    std::stringstream os;
    makeFuncOrMethodDoc(os, func.dump());
    return os.str();
}

void MakeEmmyLuaDoc::makeFuncOrMethodDoc(std::ostream &os, const GAny &methodJson)
{
    if (!methodJson.isObject()) {
        return;
    }
    if (!methodJson.contains("overloads")) {
        return;
    }
    GString fullMethodName = methodJson["name"].toString();
    auto nameSplits = fullMethodName.split(".");
    std::string classFullName;
    std::string name;
    if (nameSplits.size() == 1) {
        name = transformMethodName(nameSplits[0].toStdString());
    } else {
        classFullName = nameSplits[0].toStdString();
        name = transformMethodName(nameSplits[1].toStdString());
    }

    if (name.empty()) {
        return;
    }

    bool isStatic = methodJson["isStatic"].toBool();
    auto overloads = methodJson["overloads"];

    os << "\n";

    int32_t ovIndex = 0;
    for (auto it = overloads.iterator(); it.hasNext();) {
        const auto &ovi = it.next().second;
        if (!ovi.isObject()) {
            continue;
        }

        if (ovi.contains("doc") && ovi["doc"] != "") {
            if (ovIndex > 0) {
                os << "---\n";
            }
            os << transformDocString(ovi["doc"].toString(), "---")
                << "\n";
        }
        os << "---@overload fun(";
        const auto &args = ovi["args"];
        int32_t index = 0;
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
            if (index > 0) {
                os << ", ";
            }
            type = transformType(type);
            os << key;
            if (!type.empty()) {
                os << ":" << type;
            }
            index++;
        }
        os << ")";
        if (ovi.contains("return")) {
            std::string rType = transformType(ovi["return"].toString());
            if (!rType.empty()) {
                os << ":" << rType;
            }
        }
        os << "\n";
        ++ovIndex;
    }

    if (isLuaControlFlowKeyword(name)) {
        os << classFullName << "[\"" << name << "\"]" << " = function(...)\n"
            << "end\n";
    } else {
        if (!classFullName.empty()) {
            classFullName = classFullName + (isStatic && name != "new" ? "." : ":");
        }
        os << "function " << classFullName << name << "(...)\n"
            << "end\n";
    }
}

std::string MakeEmmyLuaDoc::transformMethodName(const std::string &name)
{
    if (name == MetaFunctionNames[static_cast<size_t>(MetaFunction::Init)]) {
        return "new";
    }
    const GString sName = name;

    // Hidden operators
    if (sName.startWith("__")) {
        return "";
    }

    return name;
}

std::string MakeEmmyLuaDoc::transformType(const std::string &type)
{
    GString sType = type;
    sType = sType.toLower();
    if (sType.indexOf(" ") >= 0) {
        return "";
    }
    if (sType == "void") {
        return "";
    }
    if (sType == "undefined") {
        return "";
    }

    // TODO: 处理指针

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
        return "nil";
    }
    if (sType == "nil") {
        return "nil";
    }

    return type;
}

std::string MakeEmmyLuaDoc::transformDocString(const std::string &doc, const std::string &commentPrefix)
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
}
