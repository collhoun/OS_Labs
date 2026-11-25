#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

typedef struct {
    void *handle;
    int index;
} Library;

static const char *const paths[] = {"./liblab1.so", "./liblab2.so"};

static void *find_function(void *handle, const char *name)
{
    dlerror();
    void *address = dlsym(handle, name);
    const char *error = dlerror();

    if (error != NULL || address == NULL) {
        fprintf(stderr, "%s: %s\n", name,
                error != NULL ? error : "null function address");
        return NULL;
    }
    return address;
}

static int load_library(Library *library, int index, Functions *functions)
{
    void *handle = dlopen(paths[index], RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", paths[index], dlerror());
        return 0;
    }

    Functions next;
    next.derivative = (DerivativeFn)find_function(handle, "Derivative");
    next.translate = (TranslationFn)find_function(handle, "translation");

    if (next.derivative == NULL || next.translate == NULL) {
        dlclose(handle);
        return 0;
    }

    if (library->handle != NULL) {
        dlclose(library->handle);
    }

    *library = (Library){handle, index};
    *functions = next;
    return 1;
}

static void switch_library(Functions *functions, void *context)
{
    Library *library = context;

    if (load_library(library, 1 - library->index, functions)) {
        fprintf(stderr, "Active library: %s\n", paths[library->index]);
    }
}

int main(void)
{
    Library library = {0};
    Functions functions;

    if (!load_library(&library, 0, &functions)) {
        return EXIT_FAILURE;
    }

    int status = run_commands(&functions, switch_library, &library);
    dlclose(library.handle);
    return status;
}