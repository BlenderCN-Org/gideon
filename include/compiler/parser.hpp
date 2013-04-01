#ifndef RT_GIDEON_PARSER_HPP
#define RT_GIDEON_PARSER_HPP

#include "compiler/ast/global.hpp"

namespace raytrace {

  namespace ast {
    
    /* Data that will be passed to the parser. */
    struct gideon_parser_data {
      parser_state *state;
      std::vector<global_declaration_ptr> *globals;
      std::vector<std::string> *dependencies;
    };

  };

};

#endif
