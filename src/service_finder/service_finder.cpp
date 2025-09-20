#include "service_finder.h"


std::unordered_map<std::type_index, void*> BT::service_finder::internal::s_services_map;
