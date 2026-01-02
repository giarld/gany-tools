//
// Created by Gxin on 25-6-5.
//

#include "gx/tools/make_js_doc.h"

#include <gx/gstring.h>


namespace tools
{
std::string MakeJsDoc::makeClassDoc(const GAnyClass &clazz)
{
    GAny dumpObj = clazz.dump();
    if (!dumpObj.isObject()) {
        return "";
    }

    std::string className = dumpObj["class"].toString();
    const std::string &fullName = className;

    std::stringstream os;

    os << "/**\n";
    if (dumpObj.contains("doc") && dumpObj["doc"] != "") {
        os << transformDocString(dumpObj["doc"].toString(), " * ") << "\n";
    }
    os << " * @class " << fullName << "\n"
        << " * @namespace " << dumpObj["nameSpace"].toString() << "\n"
        << " **/\n"
        << "class " << fullName;

    if (dumpObj.contains("parents") && dumpObj["parents"].size() > 0) {
        os  << " extends ";
        for (size_t i = 0 ; i < dumpObj["parents"].size() ; i++) {
            if (i > 0) {
                os << ", ";
            }
            os << dumpObj["parents"][i].toString();
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
            os << "    " << pName << " = " << v << ";";
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

            os << "    " << pName << ";";
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
                os << methodName << "(";

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
                        key = "...args";
                    }

                    os << key;
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
    return "";
}

std::string MakeJsDoc::transformType(const std::string &type)
{
    GString sType = type;
    sType = sType.toLower();

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
    if (sType == "GAny") {
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
