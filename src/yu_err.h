#ifndef YU_INTERNAL_INCLUDES
#error "Don't include yu_err.h directly! Include yu_common.h instead."
#endif

#define YU_ERR_LIST(X) \
  X(YU_OK, "") \
  X(YU_ERR_ALLOC_FAIL, "Allocation failure") \
  X(YU_ERR_BAD_STRING_ENCODING, "Invalid UTF8 encoded string") \
  X(YU_ERR_STRING_INDEX_OUT_OF_BOUNDS, "String index out-of-bounds") \
  X(YU_ERR_SCALAR_OPERAND_TOO_BIG, "Attempt to use a scalar-vector operation with a scalar value that would cause an overflow.") \
  X(YU_ERR_UNKNOWN, "Unknown error") \
  X(YU_ERR_UNKNOWN_FATAL, "Fatal unknown error")

DEF_ENUM(yu_err, YU_ERR_LIST)

extern const char *yu_err_messages[];

YU_INLINE
const char *yu_err_msg(yu_err err) {
    return yu_err_messages[err];
}

typedef void (* yu_err_handler_func)(yu_err);

extern yu_err_handler_func yu_global_fatal_handler;

#define YU_ERR_RET YU_CHECK_RETURN(yu_err)

#define YU_LIST_FATALS(X) \
  X(YU_ERR_ALLOC_FAIL) \
  X(YU_ERR_UNKNOWN_FATAL)

#define YU_ERR_DEFVAR yu_err yu_local_err = 0;

#define YU_ERR_HANDLER_BEGIN \
  yu_err_handler: switch (yu_local_err) { \
  case YU_OK: break;

#define YU_CATCH(errcode) case errcode:
#define YU_CATCH_ALL default:

#define YU_HANDLE_FATALS \
    YU_LIST_FATALS(YU_CATCH) { \
      yu_global_fatal_handler(yu_local_err); \
      break; \
    }

#define YU_ERR_HANDLER_END }

#define YU_ERR_DEFAULT_HANDLER(retval) \
  YU_ERR_HANDLER_BEGIN \
  YU_HANDLE_FATALS \
  YU_CATCH_ALL return retval; \
  YU_ERR_HANDLER_END \
  return retval;

#define YU_CHECK(expr) do{ \
  if (YU_UNLIKELY((yu_local_err = (expr)))) goto yu_err_handler; \
}while(0)

#define YU_THROW(err) do{ \
  yu_local_err = err; \
  goto yu_err_handler; \
}while(0)

#define YU_THROWIF(expr, err) do{ \
  if (YU_UNLIKELY((expr))) \
    YU_THROW(err); \
}while(0)

#define YU_THROWIFNULL(expr, err) YU_THROWIF((expr) == NULL, err)
#define YU_CHECK_ALLOC(expr) YU_THROWIFNULL((expr), YU_ERR_ALLOC_FAIL)
