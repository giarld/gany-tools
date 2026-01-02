//
// Created by Gxin on 24-5-20.
//

#include "gx/tools/reg_doc_tool.h"

#include "gx/tools/make_json_doc.h"
#include "gx/tools/make_markdown.h"
#include "gx/tools/make_emmy_lua_doc.h"
#include "gx/tools/make_js_doc.h"

#include <gx/gany.h>


using namespace tools;
using namespace gany;

REGISTER_GANY_MODULE(DocTools)
{
    Class<IDocMake>("Tools", "MakeJsonDoc", "")
            .func("makeClassDoc", &IDocMake::makeClassDoc)
            .func("makeFunctionDoc", &IDocMake::makeFunctionDoc);

    Class<MakeJsonDoc>("Tools", "MakeJsonDoc", "")
            .construct<>()
            .inherit<IDocMake>();

    Class<MakeMarkdown>("Tools", "MakeMarkdown", "")
            .construct<>()
            .inherit<IDocMake>();

    Class<MakeEmmyLuaDoc>("Tools", "MakeEmmyLuaDoc", "")
            .construct<>()
            .inherit<IDocMake>();

    Class<MakeJsDoc>("Tools", "MakeJsDoc", "")
            .construct<>()
            .inherit<IDocMake>();
}