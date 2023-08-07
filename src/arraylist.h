#pragma once

// vscode is dumb
#ifndef size_t
#define size_t unsigned long long
#endif

#define new_arraylist_of(...) ({ \
    ArrayList* retval; \
    do { \
        int arg_list[] = { __VA_ARGS__ }; \
        int num_args = sizeof(arg_list) / sizeof(arg_list[0]); \
        ArrayList* arr = arraylist_new(num_args); \
        for (int i = 0; i < num_args; i++) { \
            arraylist_add(arr, arg_list[i]); \
        } \
        retval = arr; \
    } while(0); \
    retval; \
})

#define arraylist(amount) ({ \
    ArrayList result; \
    do { \
        void** array = _arraylist_alloc_element_data(amount); \
        result.element_data = array; \
        result.on_stack = true; \
        result.element_data_size = amount; \
    } while(0); \
    result; \
})

#define arraylist_of(...) ({ \
    ArrayList* retval; \
    do { \
        int arg_list[] = { __VA_ARGS__ }; \
        int num_args = sizeof(arg_list) / sizeof(arg_list[0]); \
        ArrayList arr = arraylist(num_args); \
        for (int i = 0; i < num_args; i++) { \
            arraylist_add(&arr, arg_list[i]); \
        } \
        retval = &arr; \
    } while(0); \
    *retval; \
})

#define CHECK_BOUNDS(index, size) do { \
    if ((index) < 0 || (index) >= (size)) { \
        fprintf(stderr, "Error: Out of bounds access at index %d (size: %d) in function %s\n", (index), (size), __func__); \
        return ARRAYLIST_OUT_OF_BOUNDS; \
    } \
} while(0)

typedef struct ArrayList {
    void** element_data;
    size_t element_data_size;
    size_t size;
    bool on_stack;
} ArrayList;


enum ArrayListError {
    ARRAYLIST_OUT_OF_BOUNDS = 0,
    ARRAYLIST_OUT_OF_MEMORY = -1
};

void** _arraylist_alloc_element_data(size_t size);
ArrayList* arraylist_new(size_t baseSize);
size_t arraylist_add(ArrayList* ptr, void* val);
int arraylist_get(ArrayList* ptr, size_t index, void** val);
int arraylist_remove(ArrayList* ptr, size_t index, void** prev_val);
void arraylist_free(ArrayList* ptr);
