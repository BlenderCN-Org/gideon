#ifndef GD_RL_TYPE_CONVERSION_HPP
#define GD_RL_TYPE_CONVERSION_HPP

#include "compiler/types.hpp"

namespace raytrace {

  /* A table containing type a list of valid type conversions. */
  class type_conversion_table {
  public:

    typedef boost::function<llvm::Value *(llvm::Value *,
					  llvm::Module *, llvm::IRBuilder<> &)> conversion_codegen;

    struct conversion_op {
      type_spec src_type, dst_type;
      conversion_codegen codegen;
      int cost_for_arguments;
      int cost_for_operations;
    };

    //Adds a new conversion to the table.
    void add_conversion(const type_spec &src_type, const type_spec &dst_type,
			const conversion_codegen &codegen,
			int cost_for_arguments, int cost_for_operations);

    //Returns whether or not the src type can be cast to the dst type,
    //setting the cost output arguments.
    bool can_convert(const type_spec &src_type, const type_spec &dst_type,
		     /* out */ int &cost_for_arguments,
		     /* out */ int &cost_for_operations) const;

    //Converts a value of the src type to a value of the dst type (will return an error if this is not a valid cast).
    code_value convert(const type_spec &src_type, llvm::Value *src,
		       const type_spec &dst_type,
		       llvm::Module *module, llvm::IRBuilder<> &builder) const;
    
    static void initialize_standard_conversions(type_conversion_table &table, type_table &types);

  private:

    typedef std::pair<type_spec, type_spec> conversion_key;
    typedef boost::unordered_map<conversion_key, conversion_op> conversion_table;

    conversion_table valid_conversions;

  };

  namespace conversion_llvm {
    
    type_conversion_table::conversion_codegen i_to_f(type_table &types);
    type_conversion_table::conversion_codegen f_to_i(type_table &types);

  };

};

#endif
