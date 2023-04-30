#pragma once
#include "../macro_util.h"
#include "quad.h"

u64 size_of_abstract(struct Type *t);
struct BasicBlockArr build_bba_from_st(struct SymbolTable *st);
