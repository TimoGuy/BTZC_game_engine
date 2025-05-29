#pragma once

#include <string>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::vector;


namespace BT
{

class Texture_bank
{
public:
    static uint32_t load_texture_2d_from_file(string const& fname, uint32_t channels);

    static void emplace_texture_2d(string const& name, uint32_t texture_id, bool replace = false);
    static uint32_t get_texture_2d(string const& name);

private:
    inline static vector<pair<string, uint32_t>> s_textures_2d;
};

}  // namespace BT
