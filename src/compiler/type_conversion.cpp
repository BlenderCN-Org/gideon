#include "compiler/type_conversion.hpp"
#include "compiler/llvm_helper.hpp"

#include <stdexcept>
#include <iostream>

using namespace raytrace;
using namespace std;
using namespace llvm;

type_conversion_table::type_conversion_table(type_table &types) :
  types(types)
{

}

void type_conversion_table::add_conversion(const type_spec &src_type, const type_spec &dst_type,
					   const conversion_codegen &codegen,
					   int cost_for_arguments, int cost_for_operations) {
  conversion_key key = make_pair(src_type, dst_type);

  valid_conversions[key] = conversion_op {
    src_type, dst_type, codegen,
    cost_for_arguments, cost_for_operations
  };
  
}

bool type_conversion_table::can_convert(const type_spec &src_type, const type_spec &dst_type,
					/* out */ int &cost_for_arguments,
					/* out */ int &cost_for_operations) const {
  if (*src_type == *dst_type) {
    cost_for_arguments = 0;
    cost_for_operations = 0;
    return true;
  }

  auto entry = valid_conversions.find(make_pair(src_type, dst_type));
  if (entry != valid_conversions.end()) {
    cost_for_arguments = entry->second.cost_for_arguments;
    cost_for_operations = entry->second.cost_for_operations;
    return true;
  }

  //array -> array reference
  if (src_type->is_array()) {
    type_spec array_ref_type = types.get_array_ref(src_type->element_type());
    if (array_ref_type == dst_type) {
      cost_for_arguments = 1;
      cost_for_operations = 1;
      return true;
    }
  }
  
  return false;
}

code_value type_conversion_table::convert(const type_spec &src_type, Value *src,
					  const type_spec &dst_type,
					  Module *module, IRBuilder<> &builder) const {
  if (*src_type == *dst_type) return src; //trivial conversion between same types

  auto entry = valid_conversions.find(make_pair(src_type, dst_type));
  if (entry == valid_conversions.end()) {

    //do not allow array -> array reference conversions here
    if (src_type->is_array()) {
      assert(dst_type != types.get_array_ref(src_type->element_type()));
    }

    stringstream err_ss;
    err_ss << "Invalid conversion from " << src_type->name << " to " << dst_type->name;
    return errors::make_error<errors::error_message>(err_ss.str(), 0, 0);
  }
  
  return entry->second.codegen(src, module, builder);
}

void
type_conversion_table::initialize_standard_conversions(type_conversion_table &table, type_table &types) {
  table.add_conversion(types["int"], types["float"], conversion_llvm::i_to_f(types), 1, 1);
  table.add_conversion(types["float"], types["int"], conversion_llvm::f_to_i(types), 1, 2);
}

/** LLVM Conversion Functions **/

code_value conversion_llvm::array_to_array_ref(Value *arr_ptr, Type *arr_type,
					       const type_spec &ref_type,
					       Module *module, IRBuilder<> &builder) {
  Type *elem_type = cast<ArrayType>(arr_type)->getElementType();
  unsigned int N = cast<ArrayType>(arr_type)->getNumElements();

  Value *v = UndefValue::get(ref_type->llvm_type());
  v = builder.CreateInsertValue(v,
				ConstantInt::get(getGlobalContext(),
						 APInt(8*sizeof(int), N, true)),
				ArrayRef<unsigned int>(0));
  v = builder.CreateInsertValue(v, builder.CreateConstGEP2_32(arr_ptr, 0, 0),
				ArrayRef<unsigned int>(1));
  return v;
}

type_conversion_table::conversion_codegen conversion_llvm::i_to_f(type_table &types) {
  Type *f_type = types["float"]->llvm_type();
  return [f_type] (Value *val, Module *, IRBuilder<> &builder) -> Value * {
    return builder.CreateSIToFP(val, f_type, "conv_i_to_f");
  };
}

type_conversion_table::conversion_codegen conversion_llvm::f_to_i(type_table &types) {
  Type *i_type = types["int"]->llvm_type();
  return [i_type] (Value *val, Module *, IRBuilder<> &builder) -> Value * {
    return builder.CreateFPToSI(val, i_type, "conv_f_to_i");
  };
}
