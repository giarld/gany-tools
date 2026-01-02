//
// Created by Gxin on 25-4-29.
//

#include "to_any_gen.h"

#include "gx/gany.h"
#include "gx/gstring.h"


std::string formatString(const std::string &value)
{
    std::string out;
    out.reserve(value.size() * 2);
    out += '"';
    for (size_t i = 0; i < value.length(); i++) {
        const char ch = value[i];
        const uint8_t uch = ch;
        switch (uch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    out += '"';
    return out;
}

std::string ToAnyGen::genReflecClassCode(const ClassInfo &classInfo)
{
    std::stringstream code;

    std::vector<EnumClassInfo> interEnumInfos;

    std::string cppClassName = classInfo.cppName;
    std::string refClassName = classInfo.name;
    if (!classInfo.outerClass.empty()) {
        cppClassName = classInfo.outerCppName + "::" + cppClassName;
        refClassName = classInfo.outerClass + classInfo.name;
    }
    refClassName = GString(refClassName).replace(".", "").toStdString();

    // Begin
    code << "Class<" << cppClassName << ">"
        << "(\"" << classInfo.ns << "\", \"" << refClassName << "\", " << formatString(classInfo.doc) << ")";

    // inherit
    for (const auto &parent: classInfo.parents) {
        code << "\n    .inherit<" << parent << ">()";
    }

    // construct
    for (const auto &construct: classInfo.constructs) {
        for (size_t oi = 0; oi < construct->overloads.size(); oi++) {
            const auto overload = construct->overloads[oi];
            code << "\n    .construct<";
            size_t argc = overload.argTypes.size();
            for (size_t i = 0; i < argc; i++) {
                if (i != 0)
                    code << ", ";
                code << overload.argTypes[i];
            }
            code << ">(";
            genFuncDoc(code, *construct, oi);
            code << ")";
        }
    }

    // enum
    for (const auto &e: classInfo.enums) {
        std::string castTo = e->castTo;
        code << "\n    .defEnum({\n";
        for (const auto &ei: e->enumItems) {
            code << "        {" << formatString(ei) << ", ";
            if (!castTo.empty()) {
                code << "static_cast<" << castTo << ">(";
            }
            code << cppClassName << "::" << e->cppName << "::" << ei;
            if (!castTo.empty()) {
                code << ")";
            }
            code << "},\n";
        }
        code << "    })";

        EnumClassInfo enumInfo{};
        enumInfo.name = refClassName + e->name;
        enumInfo.cppName = cppClassName + "::" + e->cppName;
        enumInfo.ns = classInfo.ns;
        enumInfo.castTo = e->castTo;
        enumInfo.enumItems = e->enumItems;
        enumInfo.doc = e->doc;
        interEnumInfos.push_back(enumInfo);
    }

    // constant
    for (const auto constant: classInfo.constants) {
        code << "\n    .constant(" << formatString(constant->name) << ", " << cppClassName << "::" << constant->name << ")";
    }

    // property
    for (const auto &p: classInfo.properties) {
        if (p->hasGetter || p->hasSetter) {
            code << "\n    .property(" << formatString(p->name) << ", ";
            if (p->hasGetter) {
                code << "&" << cppClassName << "::" << p->getter->name;
            } else {
                code << "GAny()";
            }
            code << ", ";
            if (p->hasSetter) {
                code << "&" << cppClassName << "::" << p->setter->name;
            } else {
                code << "GAny()";
            }
            code << ", " << formatString(p->doc);
            code << ")";
        } else if (p->packAgain) {
            code <<"\n    REF_PROPERTY_RW(" << cppClassName << ", " << p->type << ", " << p->name << ", " << formatString(p->doc) << ")";
        } else {
            code << "\n    .readWrite(" << formatString(p->name) << ", &" << cppClassName << "::" << p->name
                << ", " << formatString(p->doc)
                << ")";
        }
    }

    // func && staticFunc
    for (const auto &func: classInfo.funcs) {
        bool hasOverloads = func->overloads.size() > 1;

        for (size_t i = 0; i < func->overloads.size(); i++) {
            const auto &overload = func->overloads[i];

            std::string funcName = func->name.empty() ? overload.name : func->name;

            if (func->isStatic) {
                code << "\n    .staticFunc(";
            } else {
                code << "\n    .func(";
            }

            if (func->isMetaFunc) {
                code << "MetaFunction::" << funcName << ", ";
            } else {
                code << formatString(funcName) << ", ";
            }

            if (!hasOverloads) {
                code << "&" << cppClassName << "::" << overload.name;
            } else {
                code << "[](";
                if (!func->isStatic) {
                    code << cppClassName << " &self";
                }
                for (size_t k = 0; k < overload.argsNames.size(); k++) {
                    if (k != 0 || !func->isStatic) {
                        code << ", ";
                    }
                    code << overload.argTypes[k] << " " << overload.argsNames[k];
                }
                code << ") {\n";
                code << "        ";
                if (overload.retType != "void") {
                    code << "return ";
                }
                if (!func->isStatic) {
                    code << "self.";
                } else {
                    code << cppClassName << "::";
                }
                code << overload.name << "(";
                for (size_t k = 0; k < overload.argsNames.size(); k++) {
                    if (k != 0) {
                        code << ", ";
                    }
                    code << overload.argsNames[k];
                }
                code << ");\n";
                code << "    }";
            }
            code << ", ";
            genFuncDoc(code, *func, i);
            code << ")";
        }
    }

    code << ";";

    for (const auto &e : interEnumInfos) {
        code << "\n\n";
        code << genReflecEnumClassCode(e);
    }

    return code.str();
}

std::string ToAnyGen::genReflecEnumClassCode(const EnumClassInfo &enumClsInfo)
{
    std::stringstream code;

    if (enumClsInfo.isDefEnum) {
        code << "REF_ENUM(" << enumClsInfo.name << ", \"" << enumClsInfo.ns << "\", " << formatString(enumClsInfo.doc) << ");";
        return code.str();
    }

    std::string cppEnumClassName = enumClsInfo.cppName;
    // Begin
    code << "Class<" << cppEnumClassName << ">"
        << "(\"" << enumClsInfo.ns << "\", \"" << enumClsInfo.name << "\", " << formatString(enumClsInfo.doc) << ")";

    std::string castTo = enumClsInfo.castTo;
    code << "\n    .defEnum({\n";
    for (const auto &ei: enumClsInfo.enumItems) {
        code << "        {" << formatString(ei) << ", ";
        if (!castTo.empty()) {
            code << "static_cast<" << castTo << ">(";
        }
        code << cppEnumClassName << "::" << ei;
        if (!castTo.empty()) {
            code << ")";
        }
        code << "},\n";
    }
    code << "    })\n";

    code << "    .func(MetaFunction::ToString, [](" << cppEnumClassName << " &self) {\n";
    code << "        switch (self) {\n";
    code << "            default: break;\n";
    for (const auto &ei: enumClsInfo.enumItems) {
        code << "            case " << cppEnumClassName << "::" << ei << " : " << "return " << formatString(ei) << ";\n";
    }
    code << "        }\n";
    code << "        return \"\";\n";
    code << "    })\n";

    code << "    REF_ENUM_OPERATORS(" << cppEnumClassName << ");";

    return code.str();
}

void ToAnyGen::genFuncDoc(std::stringstream &os, const FuncInfo &funcInfo, size_t overloadIndex)
{
    os << "{.doc=" << formatString(funcInfo.doc) << ", "
        << ".args={";
    for (size_t i = 0; i < funcInfo.overloads[overloadIndex].argsNames.size(); i++) {
        if (i != 0) {
            os << ", ";
        }
        os << "\"" << funcInfo.overloads[overloadIndex].argsNames[i] << "\"";
    }
    os << "}}";
}
