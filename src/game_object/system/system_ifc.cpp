#include "system_ifc.h"

#include "../../service_finder/service_finder.h"
#include "../component_registry.h"

#include <cassert>
#include <cctype>
#include <process.h>


namespace
{

struct Token
{
    size_t str_idx;
    enum Token_type: int32_t
    {
        TOKEN_OPEN_PAREN = 0,
        TOKEN_CLOSE_PAREN,
        TOKEN_AND,
        TOKEN_OR,
        TOKEN_NOT,
        TOKEN_TEXT_STRING,
        NUM_TOKEN_TYPES
    } type;
    std::string text;
};

std::vector<Token> extract_tokens_helper_func(std::string const& query_str)
{   // Find these tokens: ["(", ")", "&&", "||", "!", "\w+\b"]
    // (In other words, mimic this regex query to extract tokens:
    // `/(?:\(|\)|\&\&|\|\||\!|\w+\b)/gm`)
    std::vector<Token> tokens;

    for (size_t str_idx = 0; str_idx < query_str.size();)
    {
        if (std::isspace(static_cast<unsigned char>(query_str.at(str_idx))))
        {   // Trim off whitespace and try again.
            str_idx++;
            continue;
        }

        // Try to find token.
        std::string_view query_strview{ std::string_view(query_str).substr(str_idx) };
        bool found_next_token{ false };

        for (int32_t token_type_idx = 0; token_type_idx < Token::NUM_TOKEN_TYPES;
                token_type_idx++)
        {
            static std::vector<std::string> const k_token_texts{ "(", ")", "&&", "||", "!" };

            switch (token_type_idx)
            {
            case Token::TOKEN_OPEN_PAREN:
            case Token::TOKEN_CLOSE_PAREN:
            case Token::TOKEN_AND:
            case Token::TOKEN_OR:
            case Token::TOKEN_NOT:
            {
                auto& token_text{ k_token_texts[token_type_idx] };

                if (query_strview.starts_with(token_text))
                {   // Add found token!
                    tokens.emplace_back(str_idx, Token::Token_type(token_type_idx), token_text);
                    str_idx += token_text.size();
                    found_next_token = true;
                }
                break;
            }
            case Token::TOKEN_TEXT_STRING:
            {   // Assume token is as long as we keep seeing [a-zA-Z0-9_]
                size_t token_text_str_length{ 0 };
                for (unsigned char c : query_strview)
                {
                    if (std::isalnum(c) || c == '_')
                    {   // Is [a-zA-Z0-9_]
                        token_text_str_length++;
                    }
                    else
                    {   // Is not [a-zA-Z0-9_], do not include in token and finish token.
                        break;
                    }
                }

                if (token_text_str_length > 0)
                {   // Add found token!
                    tokens.emplace_back(
                        str_idx,
                        Token::Token_type(token_type_idx),
                        std::string(query_strview.substr(0, token_text_str_length)));

                    str_idx += token_text_str_length;
                    found_next_token = true;
                }
                break;
            }
            }

            // Exit if found token.
            if (found_next_token)
                break;
        }

        // Assert whether the token was found. If not, then figure out the bad token.
        assert(found_next_token);
    }

    return tokens;
}

using BT::component_system::Component_list_query;

Token peek_front_token(std::vector<Token>& tokens)
{   // Make sure that the tokens are not empty here!!!
    assert(!tokens.empty());
    return tokens.front();
}

Token pop_front_token(std::vector<Token>& tokens)
{   // Make sure that the tokens are not empty here!!!
    assert(!tokens.empty());

    auto token{ peek_front_token(tokens) };
    tokens.erase(tokens.begin());

    return token;
}

size_t process_parens_group(Component_list_query& query,
                            std::vector<Token>& tokens);
size_t process_input_val_token(Component_list_query& query,
                               std::vector<Token>& tokens);

size_t process_parens_group(Component_list_query& query,
                            std::vector<Token>& tokens)
{   // Enter parens group.
    if (pop_front_token(tokens).type != Token::TOKEN_OPEN_PAREN)
    {   // Error: Did not enter parens group with a "(" properly.
        assert(false);
    }

    size_t write_idx{ process_input_val_token(query, tokens) };

    bool keep_searching{ true };
    while (keep_searching)
    {   // Read next token to see if logical op (e.g. &&, ||).
        auto next_token{ pop_front_token(tokens) };

        if (next_token.type == Token::TOKEN_AND ||
            next_token.type == Token::TOKEN_OR)
        {   // Process another input val (the one after the logical op token).
            size_t write_idx_2{ process_input_val_token(query, tokens) };

            // Create logical op connecting.
            Component_list_query::Calculation_operation new_op;
            new_op.logical_operator =
                (next_token.type == Token::TOKEN_AND
                     ? Component_list_query::Calculation_operation::OP_TYPE_AND
                     : Component_list_query::Calculation_operation::OP_TYPE_OR);
            new_op.operands[0]      = write_idx;
            new_op.operands[1]      = write_idx_2;
            new_op.result_write_idx = query.calc_table_size_required++;

            // Overwrite write idx to output of logical op.
            // @NOTE: This makes this the result of the first operand so it can be reused if
            //        continuing to find more logical ops or input vals.
            write_idx = new_op.result_write_idx;

            query.calc_operations.emplace_back(new_op);

            keep_searching = true;
        }
        else if (next_token.type == Token::TOKEN_CLOSE_PAREN)
        {   // Exit. Parens group is over.
            keep_searching = false;
        }
        else
        {   // Error: Invalid token.
            assert(false);
        }
    }

    // Return result write idx.
    return write_idx;
}

size_t process_input_val_token(Component_list_query& query,
                               std::vector<Token>& tokens)
{   // Read input val token (also check whether there's a ! in front).
    auto input_val_token{ pop_front_token(tokens) };

    bool token_has_not_op{ input_val_token.type == Token::TOKEN_NOT };
    if (token_has_not_op)
        // Pop another token so this one should be the input val now.
        input_val_token = pop_front_token(tokens);

    size_t write_idx;
    if (input_val_token.type == Token::TOKEN_TEXT_STRING)
    {
        bool token_is_const_bool{ input_val_token.text == "true" ||
                                  input_val_token.text == "false" };

        // Create input val and insert into calc table.
        if (token_is_const_bool)
        {
            Component_list_query::Input_val_const_bool new_const_bool;
            new_const_bool.const_bool       = (input_val_token.text == "true" ? true : false);
            new_const_bool.result_write_idx = query.calc_table_size_required++;

            write_idx = new_const_bool.result_write_idx;

            query.input_vals_const_bools.emplace_back(new_const_bool);
        }
        else
        {   // Lookup struct typename from component registry.
            auto type_idx{ BT::service_finder::find_service<BT::component_system::Registry>()
                               .find_component_metadata_by_typename_str(input_val_token.text)
                               .typename_id_idx };

            Component_list_query::Input_val_struct_existance new_struct_existance{
                type_idx,
                query.calc_table_size_required++
            };

            write_idx = new_struct_existance.result_write_idx;

            query.input_vals_struct_existances.emplace_back(new_struct_existance);
        }
    }
    else if (input_val_token.type == Token::TOKEN_OPEN_PAREN)
    {   // Process new parens group.
        write_idx = process_parens_group(query, tokens);
    }
    else
    {   // Error: Invalid token for input val.
        assert(false);
    }

    if (token_has_not_op)
    {   // Add NOT op.
        Component_list_query::Calculation_operation new_op;
        new_op.logical_operator = Component_list_query::Calculation_operation::OP_TYPE_NOT;
        new_op.operands[0]      = write_idx;
        new_op.result_write_idx = query.calc_table_size_required++;

        // Overwrite write idx to output of NOT op.
        write_idx = new_op.result_write_idx;

        query.calc_operations.emplace_back(new_op);
    }

    return write_idx;
}

}  // namespace


// Component_list_query
BT::component_system::Component_list_query BT::component_system::Component_list_query::
    compile_query_string(std::string const& query_string)
{
    auto query_tokens{ extract_tokens_helper_func(query_string) };

    // Assemble tokens into query.
    Component_list_query new_query;

    size_t write_idx{ process_parens_group(new_query, query_tokens) };
    new_query.final_result_read_idx = write_idx;

    return new_query;
}

bool BT::component_system::Component_list_query::query_component_list_match(
    Component_list const& comp_list) const
{
    std::vector<char> calc_table(calc_table_size_required);  // `char` is used to avoid `bool`
                                                             // template specialization (which is
                                                             // very very slow).
    // Insert input vals.
    for (auto& in_val_se : input_vals_struct_existances)
    {
        calc_table[in_val_se.result_write_idx] =
            comp_list.check_component_exists(in_val_se.struct_typename);
    }

    for (auto& in_val_cb : input_vals_const_bools)
    {
        calc_table[in_val_cb.result_write_idx] = in_val_cb.const_bool;
    }

    // Run operations.
    for (auto& calc_op : calc_operations)
    {
        switch (calc_op.logical_operator)
        {
        case Calculation_operation::OP_TYPE_NOT:
            calc_table[calc_op.result_write_idx] =
                !static_cast<bool>(calc_table[calc_op.operands[0]]);
            break;
        case Calculation_operation::OP_TYPE_AND:
            calc_table[calc_op.result_write_idx] =
                (static_cast<bool>(calc_table[calc_op.operands[0]]) &&
                 static_cast<bool>(calc_table[calc_op.operands[1]]));
            break;
        case Calculation_operation::OP_TYPE_OR:
            calc_table[calc_op.result_write_idx] =
                (static_cast<bool>(calc_table[calc_op.operands[0]]) ||
                 static_cast<bool>(calc_table[calc_op.operands[1]]));
            break;

        default:
            // Unsupported logical operator.
            assert(false);
            break;
        }
    }

    // Read result.
    return static_cast<bool>(calc_table[final_result_read_idx]);
}


BT::component_system::System_ifc::System_ifc(std::vector<Component_list_query>&& comp_list_queries)
    : m_comp_list_queries(std::move(comp_list_queries))
{
}

void BT::component_system::System_ifc::invoke_system() const
{
    auto& registry{ service_finder::find_service<Registry>() };

    Component_lists_per_query clpq;
    clpq.reserve(m_comp_list_queries.size());

    // Get component lists for each query.
    for (auto& query : m_comp_list_queries)
    {
        clpq.emplace_back(registry.query_component_lists(query));
    }

    // Call inner system with query results.
    invoke_system_inner(std::move(clpq));
}
