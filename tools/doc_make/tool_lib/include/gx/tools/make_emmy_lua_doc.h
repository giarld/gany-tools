//
// Created by Gxin on 24-5-17.
//

#ifndef GX_DOC_TOOL_MAKE_EMMY_LUA_DOC_H
#define GX_DOC_TOOL_MAKE_EMMY_LUA_DOC_H

#include "i_doc_make.h"


namespace tools
{

class GX_API MakeEmmyLuaDoc : public IDocMake
{
public:
    std::string getDocType() override
    {
        return "lua";
    }

    std::string makeClassDoc(const GAnyClass &clazz) override;

    std::string makeFunctionDoc(const GAnyFunction &func) override;

private:
    static void makeFuncOrMethodDoc(std::ostream &os, const GAny &methodJson);

    static std::string transformMethodName(const std::string &name);

    static std::string transformType(const std::string &type);

    static std::string transformDocString(const std::string &doc, const std::string &commentPrefix);
};

}

#endif //GX_DOC_TOOL_MAKE_EMMY_LUA_DOC_H
