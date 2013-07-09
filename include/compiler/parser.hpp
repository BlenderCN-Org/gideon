/*

  Copyright 2013 Curtis Andrus

  This file is part of Gideon.

  Gideon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Gideon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Gideon.  If not, see <http://www.gnu.org/licenses/>.

*/

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
