//
// Created by Gxin on 25-6-5.
//

#ifndef GX_DOC_TOOL_MAKE_JS_DOC_H
#define GX_DOC_TOOL_MAKE_JS_DOC_H

#include "i_doc_make.h"


namespace tools
{
class GX_API MakeJsDoc : public IDocMake
{
public:
    std::string getDocType() override
    {
        return "js";
    }

    std::string makeClassDoc(const GAnyClass &clazz) override;

    std::string makeFunctionDoc(const GAnyFunction &func) override;

private:
    static std::string transformType(const std::string &type);

    static std::string transformMethodName(const std::string &name);

    static std::string transformDocString(const std::string &doc, const std::string &commentPrefix);
};
} // tools

#endif //GX_DOC_TOOL_MAKE_JS_DOC_H
