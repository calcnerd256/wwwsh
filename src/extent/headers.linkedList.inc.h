/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

#include "./linkedList.struct.h"
#include "./visitor_t.typedef.h"


#include "./alloc_copy_visitor.h"
#include "./free_visitor_copy.h"
#include "./apply_visitor.h"
#include "./traverse_linked_list.h"
#include "./clean_and_free_list.h"
#include "./visit_matcher.h"
#include "./first_matching.h"
#include "./match_last.h"
#include "./last_node.h"
#include "./append.h"

#include "../linkedList/popEmptyFreeing.linkedList.h"
#include "../linkedList/removeMiddleEmptiesFreeing.linkedList.h"


#include "../linkedList/dequoid.struct.h"

#include "../linkedList/init.dequoid.h"
#include "../linkedList/append.dequoid.h"
