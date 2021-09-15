#include <stdbool.h>
#include <stdint.h>

// This header uses Vector builtins and will cause a convertion error
// if esbmc does not implement vector extension
#include <immintrin.h>

int main(void) {
    return 0;
}