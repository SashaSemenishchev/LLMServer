#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "arraylist.h"

void** _arraylist_alloc_element_data(size_t size) {
    return (void**) malloc(size * sizeof(void*));
}

ArrayList* arraylist_new(size_t baseSize) {
    void** array = _arraylist_alloc_element_data(baseSize);
    ArrayList* result = (ArrayList*) malloc(sizeof(ArrayList));
    result->element_data = array;
    result->element_data_size = baseSize;
    result->on_stack = false;
    return result;
}

void _arraylist_grow(ArrayList* ptr, size_t by) {
    size_t old_size = ptr->element_data_size;
    size_t new_size = old_size + by;
    void** old_edata = ptr->element_data;
    void** new_edata = _arraylist_alloc_element_data(new_size);
    memcpy(new_edata, old_edata, old_size * sizeof(void*));
    ptr->element_data_size = new_size;
    ptr->element_data = new_edata;
    free(old_edata);
}

void _arraylist_shrink_single(ArrayList* ptr, size_t forget_index) {
    size_t old_size = ptr->element_data_size;
    size_t new_size = old_size - 1;
    void** old_edata = ptr->element_data;
    void** new_edata = _arraylist_alloc_element_data(new_size);
    memcpy(new_edata, old_edata, forget_index);
    memcpy(new_edata + forget_index, old_edata + forget_index + 1, old_size - forget_index - 1);
    ptr->element_data_size = new_size;
    ptr->element_data = new_edata;
    free(old_edata);
}

size_t arraylist_add(ArrayList* ptr, void* val) {
    long diff = (long)ptr->size - (long)ptr->element_data_size + 1;
    if(diff > 0) {
        _arraylist_grow(ptr, (size_t) diff);
    } 
    size_t size = ptr->size;
    ptr->element_data[size] = val;
    ptr->size = size + 1;
    return size;
}

int arraylist_get(ArrayList* ptr, size_t index, void** callback) {
    CHECK_BOUNDS(index, ptr->size);
    if(callback) {
        *callback = ptr->element_data[index]; 
    }
    return 1;
}

int arraylist_remove(ArrayList* ptr, size_t index, void** callback) {
    CHECK_BOUNDS(index, ptr->size);
    void* val_now = ptr->element_data[index];
    if(callback) {
        *callback = val_now;
    }
    size_t new_size = ptr->size - 1;
    if(index == new_size) {
        ptr->element_data[index] = NULL;
    } else {
        _arraylist_shrink_single(ptr, index);
    }
    
    ptr->size = new_size;
}

int arraylist_set(ArrayList* ptr, size_t index, void* value, void** callback) {
    CHECK_BOUNDS(index, ptr->element_data_size);

    void* current = (void*) ptr->element_data[index];
    if(callback && current) {
        *callback = current;
    }

    ptr->element_data[index] = value;
    return 1;
}

void arraylist_free(ArrayList* ptr) {
    free(ptr->element_data);
    if(!ptr->on_stack) {
        free(ptr);
    }
}