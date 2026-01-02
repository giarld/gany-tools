//
// Created by Gxin on 24-5-17.
//

#ifndef GX_DOC_TOOL_I_DOC_MAKE_H
#define GX_DOC_TOOL_I_DOC_MAKE_H

#include <gx/gany.h>

#include <ostream>


namespace tools
{

class IDocMake
{
public:
    virtual std::string getDocType() = 0;

    virtual std::string makeClassDoc(const GAnyClass &clazz) = 0;

    virtual std::string makeFunctionDoc(const GAnyFunction &func) = 0;
};

}

#endif //GX_DOC_TOOL_I_DOC_MAKE_H
