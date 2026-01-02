//
// Created by Gxin on 24-5-17.
//

#include "gx/tools/make_markdown.h"

#include <gx/gstring.h>


namespace tools
{
std::string MakeMarkdown::makeClassDoc(const GAnyClass &clazz)
{
    std::stringstream os;

    os << "# Class " << clazz.getName() << ":\n";
    os << "\n## NameSpace: \n" << clazz.getNameSpace() << "\n";
    os << "\n## Doc: \n" << clazz.getDoc() << "\n";
    if (!clazz.getParents().empty()) {
        os << "\n## Base class:\n";
        for (const GAny &p: clazz.getParents()) {
            if (p.isClass()) {
                os << "* " << p.as<GAnyClass>()->getName() << "\n";
            }
        }
    }

    GAny dumpObj = clazz.dump();

    if (dumpObj.isObject()) {
        os << "\n## Method defined:\n";
        if (dumpObj.contains("methods")) {
            for (auto it = dumpObj["methods"].iterator(); it.hasNext();) {
                auto item = it.next().second;
                os << "```cpp\n" << item["doc"].toString() << "\n```\n";
            }
        }
        os << "\n";

        os << "## Property defined:\n";
        if (dumpObj.contains("properties")) {
            for (auto it = dumpObj["properties"].iterator(); it.hasNext();) {
                auto item = it.next().second;
                os << "### " << item["name"].toString() << "\n";
                os << "- Doc: " << item["doc"].toString() << "\n";
                os << "- Type: " << transformType(item["type"].toString());
                if (item.contains("getter") && item["getter"].toBool()) {
                    os << "\n" << "- [x] Get";
                } else {
                    os << "\n" << "- [ ] Get";
                }
                if (item.contains("setter") && item["setter"].toBool()) {
                    os << "\n" << "- [x] Set";
                } else {
                    os << "\n" << "- [ ] Set";
                }
                os << "\n";
            }
        }
        os << "\n";
        os << "## Constant defined:\n";
        if (dumpObj.contains("constants")) {
            GAny constants = dumpObj["constants"];
            if (constants.size() > 0) {
                os << "```";
                for (auto it = constants.iterator(); it.hasNext();) {
                    auto item = it.next().second;
                    os << "\n" << item["name"].toString() << " = " << item["value"].toJsonString(2) << "\n";
                }
                os << "```\n";
            }
        }
    }

    return os.str();
}

std::string MakeMarkdown::makeFunctionDoc(const GAnyFunction &func)
{
    std::stringstream os;
    os << "```cpp\n" << func << "\n```";
    return os.str();
}

std::string MakeMarkdown::transformType(const std::string &type)
{
    GString sType = type;
    sType = sType.toLower();

    if (sType.indexOf("function") >= 0) {
        return "function";
    }
    if (sType.indexOf("std") >= 0) {
        if (sType.indexOf("vector") >= 0) {
            return "array";
        }
        if (sType.indexOf("list") >= 0) {
            return "array";
        }
        if (sType.indexOf("map") >= 0) {
            return "object";
        }
    }

    return type;
}
}
