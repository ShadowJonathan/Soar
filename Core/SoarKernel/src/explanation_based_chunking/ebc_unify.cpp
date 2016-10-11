/*
 * variablization_manager_map.cpp
 *
 *  Created on: Jul 25, 2013
 *      Author: mazzin
 */

#include "ebc.h"

#include "agent.h"
#include "condition.h"
#include "dprint.h"
#include "instantiation.h"
#include "preference.h"
#include "print.h"
#include "rhs.h"
#include "rhs_functions.h"
#include "symbol.h"
#include "symbol_manager.h"
#include "test.h"
#include "working_memory.h"

#include <assert.h>

bool Explanation_Based_Chunker::in_null_identity_set(test t)
{
    std::unordered_map< uint64_t, uint64_t >::iterator iter = (*unification_map).find(t->identity);
    if (iter != (*unification_map).end())
    {
        dprint(DT_UNIFICATION, "...found variablization unification %u -> %u\n",
            t->identity, iter->second);

        return (iter->second == NULL_IDENTITY_SET);
    }
    return (t->identity == NULL_IDENTITY_SET);
}

uint64_t Explanation_Based_Chunker::get_identity(uint64_t pID)
{
    std::unordered_map< uint64_t, uint64_t >::iterator iter = (*unification_map).find(pID);
    if (iter != (*unification_map).end())
    {
        dprint(DT_UNIFICATION, "...found variablization unification %u -> %u\n",
            pID, iter->second);

       return iter->second;
    }
    return pID;
}

void Explanation_Based_Chunker::unify_identity(test t)
{
    if (!ebc_settings[SETTING_EBC_LEARNING_ON]) return;
    t->identity = get_identity(t->identity);
}

void Explanation_Based_Chunker::unify_preference_identities(preference* lPref)
{
    if (!ebc_settings[SETTING_EBC_LEARNING_ON]) return;

    rhs_value lRHS;
    if (lPref->o_ids.id) lPref->o_ids.id = get_identity(lPref->o_ids.id);
    if (lPref->o_ids.attr) lPref->o_ids.attr = get_identity(lPref->o_ids.attr);
    if (lPref->o_ids.value) lPref->o_ids.value = get_identity(lPref->o_ids.value);
    if (lPref->o_ids.referent) lPref->o_ids.referent = get_identity(lPref->o_ids.referent);
    if (lPref->rhs_funcs.id)
    {
        lRHS = copy_rhs_value(thisAgent, lPref->rhs_funcs.id, true);
        deallocate_rhs_value(thisAgent, lPref->rhs_funcs.id);
        lPref->rhs_funcs.id = lRHS;
        lRHS = copy_rhs_value(thisAgent, lPref->rhs_funcs.attr, true);
        deallocate_rhs_value(thisAgent, lPref->rhs_funcs.attr);
        lPref->rhs_funcs.attr = lRHS;
        lRHS = copy_rhs_value(thisAgent, lPref->rhs_funcs.value, true);
        deallocate_rhs_value(thisAgent, lPref->rhs_funcs.value);
        lPref->rhs_funcs.value = lRHS;
    }

}
void Explanation_Based_Chunker::update_unification_table(uint64_t pOld_o_id, uint64_t pNew_o_id, uint64_t pOld_o_id_2)
{
    std::unordered_map< uint64_t, uint64_t >::iterator iter;

    for (iter = unification_map->begin(); iter != unification_map->end(); ++iter)
    {

        if ((iter->second == pOld_o_id) || (pOld_o_id_2 && (iter->second == pOld_o_id_2)))
        {
            dprint(DT_UNIFICATION, "...found secondary o_id unification mapping that needs updated: %u = %u -> %u = %u.\n", iter->first, iter->second, iter->first, pNew_o_id );
            (*unification_map)[iter->first] = pNew_o_id;
        }
    }
}

void Explanation_Based_Chunker::add_identity_unification(uint64_t pOld_o_id, uint64_t pNew_o_id)
{
    assert(pOld_o_id);
    if (pOld_o_id == pNew_o_id)
    {
        dprint(DT_UNIFICATION, "Attempting to unify identical conditions for identity %u].  Skipping.\n", pNew_o_id);
        return;
    }
    dprint(DT_UNIFICATION, "Adding identity unification %u -> %u\n", pOld_o_id, pNew_o_id);

    std::unordered_map< uint64_t, uint64_t >::iterator iter;
    uint64_t newID;

    if (pNew_o_id == 0)
    {
        /* Do not check if a unification already exists if we're propagating back a literalization */
        dprint(DT_UNIFICATION, "Adding literalization substitution: %y[%u] -> 0.\n", get_ovar_for_o_id(pOld_o_id), pOld_o_id);
        newID = 0;
    } else {
        /* See if a unification already exists for the new identity propagating back*/
        iter = (*unification_map).find(pNew_o_id);

        if (iter == (*unification_map).end())
        {
            /* Map all cases of this identity with its parent identity */
            dprint(DT_UNIFICATION, "Did not find existing mapping for %u.  Adding %y[%u] -> %y[%u].\n", pNew_o_id, get_ovar_for_o_id(pOld_o_id), pOld_o_id, get_ovar_for_o_id(pNew_o_id), pNew_o_id);
            newID = pNew_o_id;
//            dprint(DT_UNIFICATION, "Old identity propagation map:\n");
//            dprint_o_id_substitution_map(DT_UNIFICATION);
        }
        else if (iter->second == pOld_o_id)
        {
            /* Circular reference */
            dprint(DT_UNIFICATION, "o_id unification (%y[%u] -> %y[%u]) already exists.  Transitive mapping %y[%u] -> %y[%u] would be self referential.  Not adding.\n",
                get_ovar_for_o_id(pNew_o_id), pNew_o_id, get_ovar_for_o_id(iter->second), iter->second,
                get_ovar_for_o_id(pOld_o_id), pOld_o_id, get_ovar_for_o_id(iter->second), iter->second);
            return;
        }
        else
        {
            /* Map all cases of what this identity is already remapped to with its parent identity */
            dprint(DT_UNIFICATION, "o_id unification (%y[%u] -> %y[%u]) already exists.  Adding transitive mapping %y[%u] -> %y[%u].\n",
                get_ovar_for_o_id(pNew_o_id), pNew_o_id, get_ovar_for_o_id(iter->second), iter->second,
                get_ovar_for_o_id(pOld_o_id), pOld_o_id, get_ovar_for_o_id(iter->second), iter->second);
            newID = iter->second;
        }
    }

    /* See if a unification already exists for the identity being replaced in this instantiation*/
    iter = (*unification_map).find(pOld_o_id);
    uint64_t existing_mapping;
    if (iter != (*unification_map).end())
    {
        existing_mapping = iter->second;
        if (existing_mapping == 0)
        {
            if (newID != 0)
            {
                /* The existing identity we're unifying with is already literalized from a different trace.  So,
                 * literalize any tests with identity of parent in this trace */
                dprint(DT_UNIFICATION, "Literalization exists for %u.  Propagating literalization substitution with %y[%u] -> 0.\n", pOld_o_id, get_ovar_for_o_id(pNew_o_id), pNew_o_id);
                (*unification_map)[newID] = 0;
                update_unification_table(newID, 0);
            } else {
                dprint(DT_UNIFICATION, "Literalizing %u which is already literalized.  Skipping %y[%u] -> 0.\n", pOld_o_id, get_ovar_for_o_id(pNew_o_id), pNew_o_id);
            }
        } else {
            if (newID == existing_mapping)
            {
                dprint(DT_UNIFICATION, "The unification %y[%u] -> %y[%u] already exists.  Skipping.\n", get_ovar_for_o_id(pOld_o_id), pOld_o_id, get_ovar_for_o_id(newID), newID);
                return;
            }
            else if (newID == 0)
            {
                /* The existing identity we're literalizing is already unified with another identity from
                 * a different trace.  So, literalize the identity, that it is already remapped to.*/
                dprint(DT_UNIFICATION, "Unification with another identity exists for %u.  Propagating literalization substitution with %y[%u] -> 0.\n", pOld_o_id, get_ovar_for_o_id(existing_mapping), existing_mapping);
                (*unification_map)[existing_mapping] = 0;
                update_unification_table(existing_mapping, 0, pOld_o_id);
            } else {
                /* The existing identity we're unifying with is already unified with another identity from
                 * a different trace.  So, unify the identity that it is already remapped to with identity
                 * of the parent in this trace */
                dprint(DT_UNIFICATION, "Unification with another identity exists for %u.  Adding %y[%u] -> %y[%u].\n", pOld_o_id, get_ovar_for_o_id(existing_mapping), existing_mapping, get_ovar_for_o_id(pNew_o_id), pNew_o_id);
                (*unification_map)[pNew_o_id] = existing_mapping;
                (*unification_map)[newID] = existing_mapping;
                update_unification_table(newID, existing_mapping);
                update_unification_table(pNew_o_id, existing_mapping);
            }
        }
    } else {
        (*unification_map)[pOld_o_id] = newID;
        update_unification_table(pOld_o_id, newID);
    }

    /* Unify identity in this instantiation with final identity */
    dprint(DT_UNIFICATION, "New identity propagation map:\n");
    dprint_o_id_substitution_map(DT_UNIFICATION);
}

void Explanation_Based_Chunker::literalize_RHS_function_args(const rhs_value rv)
{
    /* Assign identities of all arguments in rhs fun call to null identity set*/
    cons* fl = rhs_value_to_funcall_list(rv);
    rhs_function_struct* rf = static_cast<rhs_function_struct*>(fl->first);
    cons* c;

    if (rf->can_be_rhs_value)
    {
        for (c = fl->rest; c != NIL; c = c->rest)
        {
            dprint(DT_RHS_VARIABLIZATION, "Literalizing RHS function argument %r\n", static_cast<char*>(c->first));
            if (rhs_value_is_funcall(static_cast<char*>(c->first)))
            {
                literalize_RHS_function_args(static_cast<char*>(c->first));
            } else {
                assert(rhs_value_is_symbol(static_cast<char*>(c->first)));
                rhs_symbol rs = rhs_value_to_rhs_symbol(static_cast<char*>(c->first));
                if (rs->o_id && !rs->referent->is_sti())
                {
                    add_identity_unification(rs->o_id, 0);
                }
            }
        }
    }
}

void Explanation_Based_Chunker::unify_backtraced_conditions(condition* parent_cond,
                                                         const identity_triple o_ids_to_replace,
                                                         const rhs_triple rhs_funcs)
{
    if (!ebc_settings[SETTING_EBC_LEARNING_ON]) return;
    test lId = 0, lAttr = 0, lValue = 0;
    lId = parent_cond->data.tests.id_test->eq_test;
    lAttr = parent_cond->data.tests.attr_test->eq_test;
    lValue = parent_cond->data.tests.value_test->eq_test;

    if (o_ids_to_replace.id)
    {
        if (lId->identity)
        {
            dprint(DT_IDENTITY_PROP, "Found a shared identity for identifier element: %y[%u] -> %y[%u]\n", get_ovar_for_o_id(o_ids_to_replace.id), o_ids_to_replace.id,
                get_ovar_for_o_id(lId->identity), lId->identity);
        } else {
            dprint(DT_IDENTITY_PROP, "Found an identity to literalize for identifier element: %y[%u] -> %t\n", get_ovar_for_o_id(o_ids_to_replace.id), o_ids_to_replace.id, lId);
        }
        add_identity_unification(o_ids_to_replace.id, lId->identity);
//        dprint_o_id_substitution_map(DT_IDENTITY_PROP);
    }
    else if (rhs_value_is_funcall(rhs_funcs.id))
    {
        literalize_RHS_function_args(rhs_funcs.id);
    }
    else
    {
        dprint(DT_IDENTITY_PROP, "Did not unify because %s%s\n", lId->data.referent->is_sti() ? "is identifier " : "", !o_ids_to_replace.id ? "RHS pref is literal " : "");
    }
    if (o_ids_to_replace.attr)
    {
        if (lAttr->identity)
        {
            dprint(DT_IDENTITY_PROP, "Found a shared identity for attribute element: %y[%u] -> %y[%u]\n", get_ovar_for_o_id(o_ids_to_replace.attr), o_ids_to_replace.attr,
                get_ovar_for_o_id(lAttr->identity), lAttr->identity);
        } else {
            dprint(DT_IDENTITY_PROP, "Found an identity to literalize for attribute element: %y[%u] -> %t\n", get_ovar_for_o_id(o_ids_to_replace.attr), o_ids_to_replace.attr, lAttr);
        }
        add_identity_unification(o_ids_to_replace.attr, lAttr->identity);
//        dprint_o_id_substitution_map(DT_IDENTITY_PROP);
    }
    else if (rhs_value_is_funcall(rhs_funcs.attr))
    {
        literalize_RHS_function_args(rhs_funcs.attr);
    }
    else
    {
        dprint(DT_IDENTITY_PROP, "Did not unify because %s%s\n", lAttr->data.referent->is_sti() ? "is STI " : "", !o_ids_to_replace.attr ? "RHS pref is literal " : "");
    }
    if (o_ids_to_replace.value)
    {
        if (lValue->identity)
        {
            dprint(DT_IDENTITY_PROP, "Found a shared identity to replace for value element: %y[%u] -> %y[%u]\n", get_ovar_for_o_id(o_ids_to_replace.value), o_ids_to_replace.value,
                get_ovar_for_o_id(lValue->identity), lValue->identity);
//            if ((o_ids_to_replace.value == 14) && (lValue->identity == 23))
//            {
//                dprint(DT_IDENTITY_PROP, "Found.\n");
//            }
        } else {
            dprint(DT_IDENTITY_PROP, "Found an identity to literalize for value element: %y[%u] -> %t\n", get_ovar_for_o_id(o_ids_to_replace.value), o_ids_to_replace.value, lValue);
        }
        add_identity_unification(o_ids_to_replace.value, lValue->identity);
//        dprint_o_id_substitution_map(DT_IDENTITY_PROP);
    }
    else if (rhs_value_is_funcall(rhs_funcs.value))
    {
        literalize_RHS_function_args(rhs_funcs.value);
    }
    else
    {
        dprint(DT_IDENTITY_PROP, "Did not unify because %s%s\n", lValue->data.referent->is_sti() ? "is STI " : "", !o_ids_to_replace.value ? "RHS pref is literal " : "");
    }
}