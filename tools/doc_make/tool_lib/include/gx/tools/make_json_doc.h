//
// Created by Gxin on 24-5-20.
//

#ifndef GX_DOC_TOOL_MAKE_JSON_DOC_H
#define GX_DOC_TOOL_MAKE_JSON_DOC_H

#include "i_doc_make.h"


namespace tools
{

class GX_API MakeJsonDoc : public IDocMake
{
public:
    std::string getDocType() override
    {
        return "json";
    }

    std::string makeClassDoc(const GAnyClass &clazz) override
    {
        return clazz.dump().toJsonString(2);
    }

    std::string makeFunctionDoc(const GAnyFunction &func) override
    {
        return func.dump().toJsonString(2);
    }
};

}

#endif //GX_DOC_TOOL_MAKE_JSON_DOC_H
