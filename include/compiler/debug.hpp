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

#ifndef GD_COMPILER_DEBUGGING_HPP
#define GD_COMPILER_DEBUGGING_HPP

#include "llvm/DIBuilder.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/IRBuilder.h"

#include <vector>

namespace raytrace {

  /* Forward Declarations */
  struct function_entry;

  /* Tracks the state of debug info emission. */
  class debug_state {
  public:

    debug_state(llvm::Module *m,
		const std::string &file_name, const std::string &file_path,
		bool do_optimize, bool do_emit_debug);

    static const std::string producer_name;

    void push_function(const function_entry &func,
		       unsigned int line_no, unsigned int column_no);

    void pop();

    void set_location(llvm::IRBuilder<> &builder,
		      unsigned int line_no, unsigned int column_no);

  private:

    llvm::DIBuilder dbg_builder;
    llvm::DIFile file_info;
    
    std::vector<llvm::MDNode *> state;
    

  };

};

#endif
