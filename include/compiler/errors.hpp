#ifndef GD_RL_ERRORS_HPP
#define GD_RL_ERRORS_HPP

#include <boost/variant.hpp>
#include <boost/function.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/tuple/tuple.hpp>
#include <tuple>

#include "llvm/Module.h"

#include <vector>
#include <stdexcept>
#include <boost/type_traits.hpp>

namespace raytrace {

  typedef std::runtime_error compile_error;
  
  template<typename ValueType, typename ErrorType>
  struct codegen {
    typedef boost::variant<ValueType, ErrorType> value;
    typedef boost::variant<std::nullptr_t, ErrorType> empty;

    typedef boost::tuple<ValueType, ValueType> pair;
    typedef boost::variant<pair, ErrorType> binary_value;

    typedef boost::variant<std::vector<ValueType>, ErrorType> vector;
  };

  typedef codegen<llvm::Value*, compile_error>::value codegen_value;
  typedef codegen<llvm::Value*, compile_error>::empty codegen_void;
  typedef codegen<llvm::Value*, compile_error>::pair codegen_pair;
  typedef codegen<llvm::Value*, compile_error>::binary_value codegen_binary_value;  
  typedef codegen<llvm::Value*, compile_error>::vector codegen_vector;
  
  namespace errors {

    /* Helper struct for gathering a variant's type data. */
    template<typename ContainerType>
    struct value_helper {
      typedef typename boost::mpl::at< typename ContainerType::types, boost::mpl::int_<0> >::type value_type;
      typedef typename boost::mpl::at< typename ContainerType::types, boost::mpl::int_<1> >::type error_type;
    };
    
    /* Operates an a value container type. Error types must be the same. */
    template<typename ContainerType, typename ResultType = ContainerType>
    class value_container_operation : public boost::static_visitor<ResultType> {
    public:
      
      typedef typename value_helper<ContainerType>::value_type value_type;
      typedef typename value_helper<ContainerType>::error_type error_type;
      
      typedef boost::function<ResultType (value_type &)> op_type;

      value_container_operation(const op_type &func) : func(func) { }

      ResultType operator()(value_type &val) const { return func(val); }
      ResultType operator()(error_type &error) const { return error; }

    private:

      op_type func;

    };

    class check_errors : public boost::static_visitor<> {
    public:

      void operator()(llvm::Value *&) const { std::cout << "VALUE!" << std::endl; }
      void operator()(compile_error &e) const { std::cout << "ERROR: " << e.what() << std::endl; }

    };

    /* Operates on the error in a container type. */
    template<typename ContainerType, typename ResultType = ContainerType>
    class error_container_operation : public boost::static_visitor<ResultType> {
    public:
      
      typedef typename boost::mpl::at< typename ContainerType::types, boost::mpl::int_<0> >::type value_type;
      typedef typename boost::mpl::at< typename ContainerType::types, boost::mpl::int_<1> >::type error_type;
      
      typedef boost::function<ResultType (error_type &)> op_type;

      error_container_operation(const op_type &func) : func(func) { }

      ResultType operator()(value_type &val) const { return val; }
      ResultType operator()(error_type &error) const { return func(error); }

    private:

      op_type func;

    };

    /* Extracts a value from a codegen_value or throws and exception in case of error. */
    template<typename T>
    class return_or_throw : public boost::static_visitor<T> {
    public:

      T operator()(T &v) const { return v; }
      T operator()(compile_error &e) const { throw e; }
      
    };
    
    llvm::Value *codegen_value_extract(codegen_value &val);
    codegen_void codegen_ignore_value(codegen_value &val);

    template<typename ContainerType>
    typename value_helper<ContainerType>::value_type extract_left(ContainerType &c) {
      typedef typename value_helper<ContainerType>::value_type left_type;
      return boost::apply_visitor(return_or_throw<left_type>(), c);
    }
    
    template<typename ErrorType>
    ErrorType merge_errors(const ErrorType &e0, const ErrorType &e1);

    template<typename T>
    struct is_tuple {
      typedef typename boost::mpl::bool_<false> type;
    };
    
    /* Combines two stateful values into a cons-ed type. They must share the same error type. */
    template<typename T0, typename T1> 
    struct joined_types {
      typedef typename boost::mpl::at< typename T0::types, boost::mpl::int_<0> >::type value0_type;
      typedef typename boost::mpl::at< typename T0::types, boost::mpl::int_<1> >::type error0_type;

      typedef typename boost::mpl::at< typename T1::types, boost::mpl::int_<0> >::type value1_type;
      typedef typename boost::mpl::at< typename T1::types, boost::mpl::int_<1> >::type error1_type;
      
      static_assert(boost::is_same<error0_type, error1_type>::value, "The two values must have the same error type.");
    };

    template<typename T0, typename T1>
    class join : public boost::static_visitor< boost::variant< boost::tuples::cons< typename joined_types<T0, T1>::value0_type,
										    typename joined_types<T0, T1>::value1_type >,
							       typename joined_types<T0, T1>::error0_type > > {
    public:

      typedef joined_types<T0, T1> joined_type;
      typedef typename joined_type::error0_type error_type;
      typedef typename joined_type::value0_type type0;
      typedef typename joined_type::value1_type type1;

      typedef boost::tuples::cons< type0, type1 > result_val_type;
      typedef boost::variant< result_val_type, error_type > result_type;

      result_type operator()(type0 &, error_type &e) const { return e; }
      result_type operator()(error_type &e, type1 &) const { return e; }
      result_type operator()(type0 &v0, type1 &v1) const { return result_val_type(v0, v1); }
      result_type operator()(error_type &e0, error_type &e1) { return merge_errors<error_type>(e0, e1); }
      
    };

    /* Merges an arbitrary number of values into a tuple value. */

    template<typename... ArgTypes>
    struct argument_value_join;

    template<typename T0>
    struct argument_value_join<T0> {

      typedef typename boost::mpl::at< typename T0::types, boost::mpl::int_<0> >::type value_type;
      typedef typename boost::mpl::at< typename T0::types, boost::mpl::int_<1> >::type error_type;
      typedef boost::tuples::cons<value_type, boost::tuples::null_type> result_value_type;
      typedef boost::variant<result_value_type, error_type> result_type;

      static result_type build(T0 &t) {
	auto op = [] (value_type &v) -> result_type { return v; };
	value_container_operation<T0, result_type> promote(op);
	return boost::apply_visitor(promote, t);
      }
      
    };

    template<typename T0, typename... TTail>
    struct argument_value_join<T0, TTail...> {

      typedef join<T0, typename argument_value_join<TTail...>::result_type> join_type;
      typedef typename join_type::result_val_type result_value_type;
      typedef typename join_type::result_type result_type;
      
      static result_type build(T0 &t, TTail &... tail) {
	join_type j;
	typename argument_value_join<TTail...>::result_type t_val = argument_value_join<TTail...>::build(tail...);
	return boost::apply_visitor(j, t, t_val);
      };

    };

    template<typename... ArgTypes>
    typename argument_value_join<ArgTypes...>::result_type combine_arg_list(ArgTypes &... args) {
      return argument_value_join<ArgTypes...>::build(args...);
    }
    
    /* If the two codegen_void values are empty, does nothing. Merges their errors if in an error state. */
    class merge_void : public boost::static_visitor<codegen_void> {
    public:

      codegen_void operator()(std::nullptr_t &, std::nullptr_t &) const { return nullptr; }
      codegen_void operator()(compile_error &e, std::nullptr_t &) const { return e; }
      codegen_void operator()(std::nullptr_t &, compile_error &e) const { return e; }
      codegen_void operator()(compile_error &e0, compile_error &e1) const;

    };
       
    template<typename T>
    class codegen_vector_append : public boost::static_visitor<typename codegen<typename value_helper<T>::value_type,
										typename value_helper<T>::error_type>::vector> {
    public:
      
      typedef typename value_helper<T>::value_type elem_type;
      typedef typename value_helper<T>::error_type error_type;

      typedef typename codegen<elem_type, error_type>::vector vector_type;

      vector_type operator()(std::vector<elem_type> &list, elem_type &val) const {
	std::vector<elem_type> result(list);
	result.push_back(val);
	return result;
      }
      
      vector_type operator()(std::vector<elem_type> &list, error_type &e) const { return e; }
      vector_type operator()(error_type &e, elem_type &val) const { return e; }
      vector_type operator()(error_type &e0, error_type &e1) const { return merge_errors<error_type>(e0, e1); }
      
    };

    /* Appends a value to a vector. */
    template<typename T>
    typename codegen<typename value_helper<T>::value_type,
		     typename value_helper<T>::error_type>::vector
    codegen_vector_push_back(typename codegen<typename value_helper<T>::value_type,
					      typename value_helper<T>::error_type>::vector &vec,
			     T &val) {
      codegen_vector_append<T> appender;
      return boost::apply_visitor(appender, vec, val);
    }

    /* Applies a function to the value contained in the given object. */
    template<typename T, typename FuncType>
    T codegen_call(T &arg, const FuncType &func) {
      typedef boost::function<T (typename value_helper<T>::value_type &)> func_type;
      static_assert(boost::is_convertible<FuncType, func_type>::value, "Cannot convert argument 1 to a function of the correct type.");
      
      func_type f(func);
      return boost::apply_visitor(value_container_operation<T>(f), arg);
    }
    
    /* Wraps multiple arguments into a tuple which is given as an argument to the function. */
    template<typename ResultType, typename... ArgTypes>
    ResultType codegen_call_args(const boost::function<ResultType (typename argument_value_join<ArgTypes...>::result_value_type &)> &func,
				 ArgTypes &... args) {
      typedef typename argument_value_join<ArgTypes...>::result_type arg_type;
      typedef typename argument_value_join<ArgTypes...>::result_value_type arg_val_type;
      arg_type arg_list = combine_arg_list(args...);
      
      value_container_operation<arg_type, ResultType> op(func);
      return boost::apply_visitor(op, arg_list);
    }

    //Merges two void values.
    codegen_void merge_void_values(codegen_void &v0, codegen_void &v1);
  };
};

#endif
