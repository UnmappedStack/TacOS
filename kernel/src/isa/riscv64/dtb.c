//TODO: have a cleaner thing for verbosity than the weird macros to determine if the dtb tree
//should be shown
#include <isa/cpu.h>
#include <string.h>
#include <kprintf.h>
#include <limine.h>

// idk if this is bad but it works fine
uint32_t u32_big_to_little_endian(uint32_t big) {
    uint32_t small = 0;
    small |= (uint8_t)(big >> 24); // 1000 -> 0001
    small |= (uint16_t)((big >> 8) & 0xFF00); // 0100 -> 0010
    small |= ((big & 0xFF) << 24); // 0001 -> 1000
    small |= ((big & 0xFF00) << 8); // 0010 -> 0100
    return small;
}
#define endian_swap(n) u32_big_to_little_endian(n)
#define WORD_ALIGN_UP(x) ((((x) + (4-1)) / 4) * 4)
#define WRITE_INDENTS(x) for (int i = 0; i < x; i++) kprintf("  ");

//#define DTB_SHOW_TREE // uncomment to show full tree (its a lot)

// this should be moved to a limine specific thing for abstraction idk. it'll
// be in a prekernel eventually.
static volatile struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST, .revision = 4};

void dtb_init(void) {
    struct limine_dtb_response *dtb_response = dtb_request.response;
    if (!dtb_response) kpanic("no DTB available");

    DTBHeader *dtb = dtb_response->dtb_ptr;
    if (endian_swap(dtb->magic) != DTB_HEADER_MAGIC) kpanic("invalid dtb magic");
    kprintf("[DTB] version: %u\n", endian_swap(dtb->version));

    unsigned char *strings = (unsigned char*) ((uintptr_t)dtb + endian_swap(dtb->strings_offset));
    (void) strings;
    uint32_t *struct_token = (uint32_t*)((uintptr_t)dtb + endian_swap(dtb->struct_offset));
    int depth = 0;
    while (endian_swap(*struct_token) != DTB_STRUCT_END) {
        switch (endian_swap(*struct_token)) {
        case DTB_STRUCT_BEGIN_NODE:
            char *s = (char*)((uintptr_t)struct_token + sizeof(uint32_t));
            #ifdef DTB_SHOW_TREE
            WRITE_INDENTS(depth);
            kprintf("DTB node: %s\n", s);
            #endif
            struct_token += WORD_ALIGN_UP(strlen(s)+1) / sizeof(uint32_t) + 1;
            depth++;
            break;
        case DTB_STRUCT_PROP:
            DTBProp *prop = (DTBProp*)(++struct_token);
            #ifdef DTB_SHOW_TREE
            WRITE_INDENTS(depth);
            kprintf("DTB prop: %s\n", strings + endian_swap(prop->nameoff));
            #endif
            uint32_t len_bytes = WORD_ALIGN_UP(endian_swap(prop->len) + sizeof(DTBProp));
            struct_token += len_bytes/sizeof(uint32_t); 
            break;
        case DTB_STRUCT_NOP:
            struct_token++;
            break;
        case DTB_STRUCT_END_NODE:
            depth--;
            #ifdef DTB_SHOW_TREE
            WRITE_INDENTS(depth);
            #endif
            struct_token++;
            break;
        default:
            kprintf("Unexpected structure in DTB: %u\n", endian_swap(*struct_token));
            FREEZE_DEVICE();
        }
    }

    kprintf("\nDTB init OK\n");
}
