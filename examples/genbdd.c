#include <sylvan.h>
#include <sylvan_int.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "lfi.h"

size_t map_value(size_t in){
    size_t order[] = {
        31,
        25, 26, 27, 28,
        29, 30,
        24, 23, 22, 21, 20, 19, 18, 17, 16,
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
    };
    
    return order[in];
}


MTBDD
add_value(uint32_t val)
{
    MTBDD total = sylvan_true;
    for (size_t i = 0; i < 32; i++) {
        uint32_t bit = (val >> i) & 1;
        MTBDD var = sylvan_ithvar(map_value(i));
        if (!bit) {
            var = sylvan_not(var);
        }
        total = sylvan_and(total, var);
    }
    
    
    /* MTBDD total = sylvan_true;
    MTBDD var = sylvan_ithvar(15);
    total = sylvan_and(total, var); */
    
    return total;
}

int
main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf("usage: %s out.bdd\n", argv[0]);
        return 1;
    }

    size_t n_verify = 0xffffffffULL + 1; // 0x0ffffffffULL + 1;

    lace_start(0, 0); // Number of workers, size of deque
    // sylvan_set_sizes(0x10000000, 0x1000000, 0x10000000, 0x10000000); // Initialize with these sizes
    // sylvan_set_sizes(0x10000, 0x1000, 0x10000, 0x10000);
    size_t max = 16LL<<30; // should be 2GB memory
    sylvan_set_limits(max, 1, 6);
    
    sylvan_init_package();
    sylvan_init_mtbdd(); // Initialize MTBDD (Multi-Terminal BDD)

    MTBDD full = sylvan_false;
    for (size_t i = 0; i < n_verify; i++) {
        if (i % 5000000 == 0) {
            fprintf(stderr, "%.1f\n", (float) i / (float) n_verify * 100);
        }
        if (lfi_verify_insn((uint32_t) i)) {
            // fprintf(stderr, "Passed Verification");
            full = sylvan_or(full, add_value(i));
        }
    }

    FILE* f = fopen(argv[1], "w");
    if (!f) {
        fprintf(stderr, "could not open %s\n", argv[1]);
        perror("open");
        return 1;
    }
    // sylvan_serialize_tofile(f); // Serialize BDD to file
    // mtbdd_fprintdot(f, full);
    
    // sylvan_serialize_add(full);
    // sylvan_serialize_totext(f);
    
    mtbdd_writer_totext(f, &full, 1);
    
    fclose(f);

    sylvan_quit();
    lace_stop();

    return 0;
}
