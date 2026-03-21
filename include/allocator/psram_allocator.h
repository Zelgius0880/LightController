//
// Created by Zelgius on 20-03-26.
//

#ifndef LIGHTCONTROLLER_PSRAM_ALLOCATOR_H
#define LIGHTCONTROLLER_PSRAM_ALLOCATOR_H
struct PsramAllocator : Allocator {
    virtual ~PsramAllocator() = default;

    void *allocate(size_t size) override { return ps_malloc(size); }
    void deallocate(void *ptr) override { free(ptr); }
    void *reallocate(void *ptr, size_t new_size) override { return ps_realloc(ptr, new_size); }
};

#endif //LIGHTCONTROLLER_PSRAM_ALLOCATOR_H