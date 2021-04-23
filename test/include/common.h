#ifndef common_h
#define common_h

#include <napi/js_native_api_types.h>

EXTERN_C_START

// Empty value so that macros here are able to return NULL or void
#define NAPI_RETVAL_NOTHING  // Intentionally blank #define

#define GET_AND_THROW_LAST_ERROR(env)                                    \
  do {                                                                   \
    const NAPIExtendedErrorInfo *errorInfo;                          \
    napi_get_last_error_info((env), &errorInfo);                        \
    bool isPending;                                                     \
    napi_is_exception_pending((env), &isPending);                       \
    /* If an exception is already pending, don't rethrow it */           \
    if (!isPending) {                                                   \
      const char* errorMessage = errorInfo->errorMessage != NULL ?    \
        errorInfo->errorMessage :                                      \
        "empty error message";                                           \
      napi_throw_error((env), NULL, errorMessage);                      \
    }                                                                    \
  } while (0)

#define NAPI_CALL_BASE(env, theCall, retVal)                           \
  do {                                                                   \
    if ((theCall) != NAPIOK) {                                         \
      GET_AND_THROW_LAST_ERROR((env));                                   \
      return retVal;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL if the_call doesn't return napi_ok.
#define NAPI_CALL(env, theCall)                                         \
  NAPI_CALL_BASE(env, theCall, NULL)

// Returns empty if the_call doesn't return napi_ok.
#define NAPI_CALL_RETURN_VOID(env, the_call)                             \
  NAPI_CALL_BASE(env, the_call, NAPI_RETVAL_NOTHING)

#define NAPI_ASSERT_BASE(env, assertion, message, retVal)               \
  do {                                                                   \
    if (!(assertion)) {                                                  \
      napi_throw_error(                                                  \
          (env),                                                         \
        NULL,                                                            \
          "assertion (" #assertion ") failed: " message);                \
      return retVal;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside napi_callback methods.
#define NAPI_ASSERT(env, assertion, message)                             \
  NAPI_ASSERT_BASE(env, assertion, message, NULL)

#define DECLARE_NAPI_PROPERTY(name, func)                                \
  { (name), NULL, (func), NULL, NULL, NULL, NAPIDefault, NULL }

#define DECLARE_NAPI_GETTER(name, func)                                  \
  { (name), NULL, NULL, (func), NULL, NULL, NAPIDefault, NULL }

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)            \
    do                              \
    {                               \
        NAPIStatus status = expr; \
        if (status != NAPIOK)       \
        {                           \
            return status;          \
        }                           \
    } while (0)

static NAPIValue strictEqual(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 2;
    NAPIValue args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 2, "Wrong number of arguments");

    bool result = false;
    NAPI_CALL(env, napi_strict_equals(env, args[0], args[1], &result));
    NAPI_ASSERT(env, result, "assert.strictEqual()");

    return NULL;
}

static NAPIValue throws(NAPIEnv env, NAPICallbackInfo info) {
    size_t argc = 1;
    NAPIValue args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

    NAPIValueType valueType;
    NAPI_CALL(env, napi_typeof(env, args[0], &valueType));
    NAPI_ASSERT(env, valueType == NAPIFunction, "Argument is not function");

    bool result = false;
    NAPI_ASSERT(env, napi_is_exception_pending(env, &result), "napi_is_exception_pending()");
    NAPI_ASSERT(env, !result, "napi_typeof() or before function throw exception");

    NAPIStatus status = napi_call_function(env, NULL, args[0], 0, NULL, NULL);
    NAPI_ASSERT(env, status == NAPIPendingException, "assert.throws()");

    return NULL;
}

static inline NAPIStatus initAssert(NAPIEnv env, NAPIValue global) {
    NAPIValue value;
    CHECK_NAPI(napi_create_object(env, &value));
    CHECK_NAPI(napi_set_named_property(env, global, "assert", value));

    NAPIPropertyDescriptor desc = DECLARE_NAPI_PROPERTY("strictEqual", strictEqual);
    CHECK_NAPI(napi_define_properties(env, value, 1, &desc));

    desc = DECLARE_NAPI_PROPERTY("throws", throws);
    CHECK_NAPI(napi_define_properties(env, value, 1, &desc));

    return NAPIOK;
}

EXTERN_C_END

#endif /* common_h */
