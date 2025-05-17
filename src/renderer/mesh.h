#pragma once

#include "cglm/cglm.h"
#include <string>

using std::string;


namespace BT
{

class Mesh
{
public:
    Mesh(string const& obj_fname);

private:
    vec3 min_bound;
    vec3 max_bound;

    void load_obj_as_mesh(string const& fname);
};

}  // namespace BT
