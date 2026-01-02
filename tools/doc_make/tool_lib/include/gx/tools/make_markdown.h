//
// Created by Gxin on 24-5-17.
//

#ifndef GX_DOC_TOOL_MAKE_MARKDOWN_H
#define GX_DOC_TOOL_MAKE_MARKDOWN_H

#include "i_doc_make.h"


namespace tools
{

class GX_API MakeMarkdown : public IDocMake
{
public:
    std::string getDocType() override
    {
        return "markdown";
    }

    std::string makeClassDoc(const GAnyClass &clazz) override;

    std::string makeFunctionDoc(const GAnyFunction &func) override;

private:
    static std::string transformType(const std::string &type);
};

}

#endif //GX_DOC_TOOL_MAKE_MARKDOWN_H
