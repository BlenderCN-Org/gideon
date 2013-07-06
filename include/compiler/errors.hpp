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

#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/include/cons.hpp>

#include "llvm/IR/Module.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <boost/type_traits.hpp>

#include <boost/fusion/functional/invocation/invoke.hpp>

namespace raytrace {

  namespace errors {
    class error;
    typedef std::shared_ptr<error> error_ptr;
  };
  
  struct compile_error {
    errors::error_ptr e;
    
    explicit compile_error(const errors::error_ptr &e) : e(e) { }

    errors::error &operator*() const { return *e; }
    errors::error *operator->() const { return e.get(); }
  };

    //typedef errors::error_ptr compile_error;
  
  struct empty_type { };

  template<typename ValueType, typename ErrorType>
  struct codegen {
    typedef boost::variant<ValueType, ErrorType> value;
    typedef boost::variant<empty_type, ErrorType> empty;

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
    
    /* Error Classes */

    //Base error class.
    class error {
    public:

      error(unsigned int line_no, unsigned int column_no);

      virtual std::string report() const = 0;
      void set_location(unsigned int line, unsigned int column);

    protected:

      unsigned int line_no, column_no;
      std::string location() const;

    };
    
    //Builds a compiler error object.
    template<typename Error, typename... ArgTypes>
    compile_error make_error(ArgTypes... args) { return compile_error(error_ptr(new Error(args...))); }

    //A pair of errors (used for implementing the merge functionality).
    class error_pair : public error {
    public:

      error_pair(const error_ptr &first, const error_ptr &second);

      virtual std::string report() const;

    private:

      error_ptr first, second;

    };

    //A container wrapping other errors into a group.
    class error_group : public error {
    public:
      
      error_group(const std::string &group_name,
		  const error_ptr &errors,
		  unsigned int line_no, unsigned int column_no);

      virtual std::string report() const;

    private:

      std::string group_name;
      error_ptr errors;

    };

    //Generic error message.
    class error_message : public error {
    public:

      error_message(const std::string &msg,
		    unsigned int line_no, unsigned int column_no);

      virtual std::string report() const;

    private:

      std::string msg;

    };

    //A reference to an undefined variable.
    class undefined_variable : public error {
    public:

      undefined_variable(const std::string &name,
			 unsigned int line_no, unsigned int column_no);

      virtual std::string report() const;

    private:
      
      std::string name;

    };

    //An invalid function call.
    class invalid_function_call : public error {
    public:
      
      invalid_function_call(const std::string &name, const std::string &error_msg,
			    unsigned int line_no, unsigned int column_no);

      virtual std::string report() const;
      
    private:

      std::string name, error_msg;
      
    };

    //Type mistmatch error.
    class type_mismatch : public error {
    public:

      type_mismatch(const std::string &type, const std::string &expected,
		    unsigned int line_no, unsigned int column_no);

      virtual std::string report() const;

    private:

      std::string type, expected;
      
    };

    /* Error Helper Functions */

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
      void operator()(compile_error &e) const { std::cout << "ERROR: " << e->report() << std::endl; }

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

    //If the value is an error, sets its location.
    template<typename ContainerType>
    void error_set_location(ContainerType &v,
			    unsigned int line_no, unsigned int column_no) {
      typedef error_container_operation<ContainerType> error_op_type;
      error_op_type tag([line_no, column_no] (typename error_op_type::error_type &e) -> ContainerType {
	  e->set_location(line_no, column_no);
	  return e;
	});
      boost::apply_visitor(tag, v);
    }

    /* Extracts a value from a codegen_value or throws and exception in case of error. */
    template<typename T>
    class return_or_throw : public boost::static_visitor<T> {
    public:

      T operator()(T &v) const { return v; }
      T operator()(compile_error &e) const { throw std::runtime_error(e->report()); }
      
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
    class join : public boost::static_visitor< boost::variant< boost::fusion::cons< typename joined_types<T0, T1>::value0_type,
										    typename joined_types<T0, T1>::value1_type >,
							       typename joined_types<T0, T1>::error0_type > > {
    public:

      typedef joined_types<T0, T1> joined_type;
      typedef typename joined_type::error0_type error_type;
      typedef typename joined_type::value0_type type0;
      typedef typename joined_type::value1_type type1;

      typedef boost::fusion::cons< type0, type1 > result_val_type;
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
      //typedef boost::fusion::cons<value_type, boost::tuples::null_type> result_value_type;
      typedef boost::fusion::cons<value_type> result_value_type;
      typedef boost::variant<result_value_type, error_type> result_type;

      static result_type build(T0 &t) {
	auto op = [] (value_type &v) -> result_type { return result_value_type(v); };
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

    template<int N, typename JoinedType>
    auto get(JoinedType &arg) -> decltype(boost::fusion::at_c<N>(arg)) {
      return boost::fusion::at_c<N>(arg);
    }
    
    /* If the two codegen_void values are empty, does nothing. Merges their errors if in an error state. */
    class merge_void : public boost::static_visitor<codegen_void> {
    public:

      codegen_void operator()(empty_type &, empty_type &) const { return empty_type(); }
      codegen_void operator()(compile_error &e, empty_type &) const { return e; }
      codegen_void operator()(empty_type &, compile_error &e) const { return e; }
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
    template<typename T, typename RT = T, typename FuncType = typename boost::function<RT (typename value_helper<T>::value_type &)> >
    RT codegen_call(T &arg, const FuncType &func) {
      typedef boost::function<RT (typename value_helper<T>::value_type &)> func_type;
      static_assert(boost::is_convertible<FuncType, func_type>::value, "Cannot convert argument 1 to a function of the correct type.");
      
      func_type f(func);
      return boost::apply_visitor(value_container_operation<T, RT>(f), arg);
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

    template<typename ResultType, typename... ArgTypes>
    ResultType codegen_apply(const boost::function<ResultType (typename errors::value_helper<ArgTypes>::value_type &...)> &func,
			     ArgTypes&... args) {
      typedef typename errors::argument_value_join<ArgTypes...>::result_value_type arg_val_type;
      boost::function<ResultType (arg_val_type &)> joined_func = [&func] (arg_val_type &arg) -> ResultType {
	return boost::fusion::invoke(func, arg);
      };
      
      return errors::codegen_call_args(joined_func, args...);
    }


    //Merges two void values.
    codegen_void merge_void_values(codegen_void &v0, codegen_void &v1);


  };
};

#endif
