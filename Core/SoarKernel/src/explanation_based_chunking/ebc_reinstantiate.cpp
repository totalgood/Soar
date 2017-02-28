#include "ebc.h"
#include "agent.h"
#include "dprint.h"
#include "explanation_memory.h"
#include "instantiation.h"
#include "condition.h"
#include "preference.h"
#include "symbol.h"
#include "symbol_manager.h"
#include "test.h"
#include "print.h"
#include "rhs.h"
#include "xml.h"

#include <assert.h>

void Explanation_Based_Chunker::reinstantiate_test (test pTest)
{
    if (pTest->type == CONJUNCTIVE_TEST)
    {
        for (cons* c = pTest->data.conjunct_list; c != NIL; c = c->rest)
        {
            reinstantiate_test(static_cast<test>(c->first));
        }
    }
    else if (test_has_referent(pTest) && pTest->data.referent->is_variable() && pTest->identity)
    {
        Symbol* oldSym = pTest->data.referent;
        pTest->data.referent = pTest->data.referent->var->instantiated_sym;
        thisAgent->symbolManager->symbol_add_ref(pTest->data.referent);
        thisAgent->symbolManager->symbol_remove_ref(&oldSym);
    }
}

void Explanation_Based_Chunker::reinstantiate_condition_list(condition* top_cond, bool pIsNCC)
{
    for (condition* cond = top_cond; cond != NIL; cond = cond->next)
    {
        reinstantiate_condition(cond, pIsNCC);
    }
}

void Explanation_Based_Chunker::reinstantiate_condition(condition* cond, bool pIsNCC)
{
    if (cond->type != CONJUNCTIVE_NEGATION_CONDITION)
    {
        reinstantiate_test(cond->data.tests.id_test);
        reinstantiate_test(cond->data.tests.attr_test);
        reinstantiate_test(cond->data.tests.value_test);
        dprint(DT_REINSTANTIATE, "Reinstantiated condition is now %l\n", cond);
        #ifdef EBC_SANITY_CHECK_RULES
        sanity_justification_test(cond->data.tests.id_test, pIsNCC);
        sanity_justification_test(cond->data.tests.attr_test, pIsNCC);
        sanity_justification_test(cond->data.tests.value_test, pIsNCC);
        #endif
    } else {
        reinstantiate_condition_list(cond->data.ncc.top, true);
    }

}

condition* Explanation_Based_Chunker::reinstantiate_lhs(condition* top_cond)
{
    dprint_header(DT_REINSTANTIATE, PrintBoth, "Reversing variablization of condition list\n");

    condition* last_cond, *lCond, *inst_top;
    last_cond = inst_top = lCond = NULL;

    for (condition* cond = m_lhs; cond != NIL; cond = cond->next)
    {
        dprint(DT_REINSTANTIATE, "Reversing variablization of condition: %l\n", cond);

        if (m_rule_type == ebc_justification)
        {
            reinstantiate_condition(cond);
            lCond = copy_condition(thisAgent, cond, false, false, false);
            lCond->inst = m_chunk_inst;
            lCond->bt = cond->bt;
        } else {
            lCond = copy_condition(thisAgent, cond, false, false, false);
            lCond->bt = cond->bt;
            lCond->inst = m_chunk_inst;
            reinstantiate_condition(lCond);
        }

        if (last_cond)
        {
            last_cond->next = lCond;
        } else {
            inst_top = lCond;
        }
        lCond->prev = last_cond;
        last_cond = lCond;
    }
    if (last_cond)
    {
        last_cond->next = NULL;
    }
    else
    {
        inst_top = NULL;
    }

    dprint_header(DT_REINSTANTIATE, PrintAfter, "Done reversing variablization of LHS condition list.\n");
    return inst_top;
}

void Explanation_Based_Chunker::reinstantiate_rhs_symbol(rhs_value pRhs_val)
{

    Symbol* var;

    if (rhs_value_is_funcall(pRhs_val))
    {
        cons* fl = rhs_value_to_funcall_list(pRhs_val);
        cons* c;

        for (c = fl->rest; c != NULL; c = c->rest)
        {
            dprint(DT_RHS_FUN_VARIABLIZATION, "Reversing variablization of funcall RHS value %r\n", static_cast<char*>(c->first));
            reinstantiate_rhs_symbol(static_cast<char*>(c->first));
            dprint(DT_RHS_FUN_VARIABLIZATION, "... RHS value is now %r\n", static_cast<char*>(c->first));
        }
        return;
    }

    rhs_symbol rs = rhs_value_to_rhs_symbol(pRhs_val);

    if (rs->referent->is_variable())
    {
        dprint(DT_REINSTANTIATE, "Reversing variablization for RHS symbol %y [%u] -> %y.\n", rs->referent, rs->identity, rs->referent->var->instantiated_sym);
        Symbol* oldSym = rs->referent;
        rs->referent = rs->referent->var->instantiated_sym;
        thisAgent->symbolManager->symbol_add_ref(rs->referent);
        thisAgent->symbolManager->symbol_remove_ref(&oldSym);
        if (rs->identity_set)
        {
            if (rs->identity_set->super_join->clone_identity) rs->identity = rs->identity_set->super_join->clone_identity;
            /* MToDo | Is there a chance this could be an unbound rhs var with a new identity that it owns?
             *         May want to place an assert outside of this function that checks the preference ownership */
            rs->identity_set = NULL;
        }
    } else {
        dprint(DT_REINSTANTIATE, "Not a variable.  Ignoring %y [%u]\n", rs->referent, rs->identity);
    }
    assert(!rs->referent->is_variable());

}

void Explanation_Based_Chunker::reinstantiate_actions(action* pActionList)
{
    for (action* lAction = pActionList; lAction != NULL; lAction = lAction->next)
    {
        if (lAction->type == MAKE_ACTION)
        {
            reinstantiate_rhs_symbol(lAction->id);
            reinstantiate_rhs_symbol(lAction->attr);
            reinstantiate_rhs_symbol(lAction->value);
            if (lAction->referent)
            {
                reinstantiate_rhs_symbol(lAction->referent);
            }
        }
    }
}

condition* Explanation_Based_Chunker::reinstantiate_current_rule()
{
    dprint(DT_REINSTANTIATE, "m_lhs before reinstantiation: \n%1", m_lhs);

    condition* returnConds = reinstantiate_lhs(m_lhs);

    dprint(DT_REINSTANTIATE, "m_lhs after reinstantiation: \n%1", m_lhs);

    dprint(DT_REINSTANTIATE, "m_rhs before reinstantiation: \n%2", m_rhs);
    if (m_rule_type == ebc_justification)
    {
        reinstantiate_actions(m_rhs);
    }
    dprint(DT_REINSTANTIATE, "m_rhs after reinstantiation: \n%2", m_rhs);

    return returnConds;
}
