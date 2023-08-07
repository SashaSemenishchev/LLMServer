#define println(format, ...) printf(format "\n", __VA_ARGS__)
#define array_size(arr) (sizeof(arr) / sizeof(*(arr)))
#define HashMap struct hashmap
#define lambda(return_type, function_body) \
({ \
      return_type __fn__ function_body \
          __fn__; \
})
