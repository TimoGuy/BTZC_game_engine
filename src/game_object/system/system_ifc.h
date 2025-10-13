////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief   Interface for a system in a bare-bones ECS (entity component system).
///          Compiles the queries used for finding entities.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <string>
#include <typeindex>
#include <vector>


namespace BT
{
namespace component_system
{

/// Forward declare.
class Component_list;

/// Compiled query used to search thru registry for entities.
struct Component_list_query
{   /// Creates an `Component_list_query` from a `query_string`.
    /// @details Example `query_string = "(Struct_1 && !(Struct_2 || Struct_3))"`.
    static Component_list_query compile_query_string(std::string const& query_string);

    /// Queries component list whether it matches the query.
    bool query_component_list_match(Component_list const& comp_list) const;




    #if 0
    struct Logic_group
    {
        enum Operator : int32_t
        {
            LOGIC_AND,
            LOGIC_OR,
        } logical_operator;

        /// Idx in `logic_terms` to write result to.
        /// @note If this is set to `-1`, then the result is the final result.
        size_t result_write_idx;
    };
    std::vector<Logic_group> logic_groups;

    struct Logic_term
    {
        enum Term_type : int32_t
        {
            TERM_CONST_TRUE,  // @NOTE: Use `BOOL_OP_NOT` to make this const False.
            TERM_STRUCT_TYPENAME,
            TERM_RESULT,
        } term_type;

        enum Boolean_op : int32_t
        {
            BOOL_OP_NORMAL,
            BOOL_OP_NOT,  // The not operator `!`.
        } bool_op;

        /// Typename to check for existance in an entity.
        /// @note Not applicable unless `TERM_STRUCT_TYPENAME`.
        std::type_index struct_typename;

        /// Stored result written by a `Logic_group`.
        /// @note Not applicable unless `TERM_RESULT`.
        bool result;
    };
    std::vector<Logic_term> logic_terms;
    #endif  // 0






    struct Input_val_struct_existance
    {
        Input_val_struct_existance(std::type_index struct_typename, size_t result_write_idx)
            : struct_typename(struct_typename)
            , result_write_idx(result_write_idx)
        {
        }

        /// Typename to check for existance in an entity.
        /// Write existance result into calculation table.
        std::type_index struct_typename;

        size_t result_write_idx;
    };
    /// Before calc ops happen, input vals are inserted
    /// (this is for checking whether a struct exists)
    std::vector<Input_val_struct_existance> input_vals_struct_existances;

    struct Input_val_const_bool
    {
        /// Constant value to write into calculation table.
        bool const_bool;

        size_t result_write_idx;
    };
    /// Before calc ops happen, input vals are inserted
    /// (this is for const bool values)
    std::vector<Input_val_const_bool> input_vals_const_bools;

    struct Calculation_operation
    {   /// Calculation operation type.
        /// Note that `OR_TYPE_NOT` only requires one operand.
        enum Op_type
        {
            OP_TYPE_NOT,  // !
            OP_TYPE_AND,  // &&
            OP_TYPE_OR,   // ||
        } logical_operator;

        /// Operands to pull for the calculation. Sometimes both aren't used.
        size_t operands[2];

        size_t result_write_idx;
    };

    /// Once all input vals are inserted, then all calculation operations are run.
    std::vector<Calculation_operation> calc_operations;

    /// Once all calc ops are run, this reads the final value for whether the query succeeded.
    size_t final_result_read_idx{ (size_t)-1 };

    /// In order to fit all the input and calculated values, this is the number of bools required
    /// for the calculation table.
    size_t calc_table_size_required{ 0 };
};

/// Interface for ECS system.
class System_ifc
{
public:
    System_ifc(std::vector<Component_list_query>&& comp_list_queries);
    virtual ~System_ifc() = default;

    /// Invokes system by querying for the required component lists and then calling
    /// `invoke_system_inner()`.
    /// @FUTURE: It would be nice if the results of the queries were cached after queries.
    void invoke_system() const;

protected:
    using Component_lists_per_query = std::vector<std::vector<Component_list*>>;
    virtual void invoke_system_inner(
        Component_lists_per_query&& comp_lists_per_query) const = 0;

private:
    std::vector<Component_list_query> m_comp_list_queries;
};

}  // namespace component_system
}  // namespace BT
