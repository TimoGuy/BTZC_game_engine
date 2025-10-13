#include "system_ifc.h"

#include <cassert>
#include <cctype>
#include <memory>


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
        TOKEN_STRUCT_TYPENAME,
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
            case Token::TOKEN_STRUCT_TYPENAME:
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

}  // namespace


// Component_list_query
BT::component_system::Component_list_query BT::component_system::Component_list_query::
    compile_query_string(std::string const& query_string)
{
    auto query_tokens{ extract_tokens_helper_func(query_string) };

    // Assemble tokens into query.
    Component_list_query new_query;

    // @TODO
    assert(false);

    return new_query;
}

bool BT::component_system::Component_list_query::query_component_list_match(
    Component_list const& comp_list) const
{
    // @TODO
    assert(false);
}


BT::component_system::System_ifc::System_ifc(std::vector<Component_list_query>&& comp_list_queries)
    : m_comp_list_queries(std::move(comp_list_queries))
{
}

void BT::component_system::System_ifc::invoke_system() const
{
    // @TODO
    assert(false);

    invoke_system_inner({});
}
