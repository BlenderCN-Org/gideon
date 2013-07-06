#ifndef GD_RL_VALUE_HPP
#define GD_RL_VALUE_HPP

#include <memory>
#include <boost/variant.hpp>
#include "llvm/IR/DerivedTypes.h"

namespace raytrace {

  class module_object;

  class value {
  public:
    
    typedef std::shared_ptr<module_object> module_ptr;

    value(llvm::Value *v);
    value(const module_ptr &module);

    llvm::Value *extract_value();
    module_ptr extract_module();

    template<typename T, typename Visitor>
    T apply_visitor(const Visitor &visitor) {
      return boost::apply_visitor(visitor, data);
    }

    typedef enum { LLVM_VALUE, MODULE_VALUE } value_type;
    value_type type() const;
    
  private:

    boost::variant<llvm::Value*, module_ptr> data;

  };

};


#endif
