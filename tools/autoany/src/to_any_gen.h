//
// Created by Gxin on 25-4-29.
//

#ifndef TO_ANY_GEN_H
#define TO_ANY_GEN_H

#include "cpp_types_info_gen.h"

#include <sstream>


class ToAnyGen
{
public:
    static std::string genReflecClassCode(const ClassInfo &classInfo);

    static std::string genReflecEnumClassCode(const EnumClassInfo &enumClsInfo);

private:
    static void genFuncDoc(std::stringstream &os, const FuncInfo &funcInfo, size_t overloadIndex);
};

#endif //TO_ANY_GEN_H
