#define _GNU_SOURCE
#include <stdio.h>

// use later for calculating statistics
static int address_references = 0;
static int page_faults = 0;
static int TLB_hits = 0;

// TLB entry structure
typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;

// intialize TLB with 16 entries
static TLBEntry tlb[16];
static int tlb_index = 0;

// intialize page table with 256 entries
static int page_table[256];
static int next_free_frame = 0;

static FILE *backing_store;

static char physical_memory[65536] = {0}; // 256 x 256 bytes

int translate_address(int logical_address) {
    logic_address = logical_address & 0xFFFF; // ignore upper 16 bits
    int offset = logical_address & 0xFF;
    logical_address = logical_address >> 8;
    int page_num = logical_address & 0xFF;

    int frame_number = -1;

    // check TLB for page number
    for (int i = 0; i < 16; i++) {
    if (tlb[i].page_number == page_num) {
        frame_number = tlb[i].frame_number;
        // TLB hit
        TLB_hits++;
        break;
        }
    }
    // check page table for page number if not found in TLB
    if (frame_number == -1) {
         if (page_table[page_num] != -1) {
            // page table hit
            frame_number = page_table[page_num];
        }
        else {
            page_faults++;
            // page fault, load page from backing store into physical memory
            frame_number = next_free_frame;
            next_free_frame++;

            // check bounds for physical memory
            if (next_free_frame >= 256) {
                printf("Physical memory full\n");
                return -1;
            }

            fseek(backing_store, page_num * 256, SEEK_SET);

            fread(&physical_memory[frame_number * 256],
                sizeof(char), 256, backing_store);

            page_table[page_num] = frame_number;
        }

        // update TLB with new page number and frame number
        tlb[tlb_index].page_number = page_num;
        tlb[tlb_index].frame_number = frame_number;

        tlb_index = (tlb_index + 1) % 16;
    }

    int physical_address = (frame_number * 256) + offset;
 
    return physical_address;
}

void main(int argc, char *argv[]) {
    // set all TLB entries to invalid
    for (int i = 0; i < 16; i++) {
        tlb[i].page_number = -1;
    }

    // initialize page table entries to invalid
    for (int i = 0; i < 256; i++) {
        page_table[i] = -1; 
    }

    // check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address_file>\n", argv[0]);
        return;
    }

    // open the backing store and address file
    backing_store = fopen("BACKING_STORE.bin", "rb");
    FILE *address_file = fopen(argv[1], "r");

    // error checking for file opening
    if (backing_store == NULL || address_file == NULL) {
        printf("Error opening files\n");
        return;
    }

    int logical_address;
    // translate each logical address (integer) from the input file
    while (fscanf(address_file, "%d", &logical_address) == 1) {

        // translate logical address
        int physical_address = translate_address(logical_address);

        char value = physical_memory[physical_address];
        // print results
        printf("Logical Address: %d, Physical Address: %d, Value: %d\n", logical_address, physical_address, value);
        address_references++;
    }

    // print statistics
    printf("\nTotal References: %d\n", address_references);
    printf("Page Faults: %d\n", page_faults);
    printf("TLB Hits: %d\n", TLB_hits);

    printf("Page Fault Rate: %.2f%%\n", (double)page_faults / address_references * 100);

    printf("TLB Hit Rate: %.2f%%\n", (double)TLB_hits / address_references * 100);
    
    fclose(backing_store);
    fclose(address_file);
}
