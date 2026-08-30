#pragma once

#define DTB_HEADER_MAGIC 0xD00DFEED

#define DTB_STRUCT_BEGIN_NODE 1
#define DTB_STRUCT_END_NODE   2
#define DTB_STRUCT_PROP       3
#define DTB_STRUCT_NOP        4
#define DTB_STRUCT_END        9

typedef struct {
	uint32_t magic;
	uint32_t size;
	uint32_t struct_offset;
	uint32_t strings_offset;
	uint32_t memory_offset;
	uint32_t version;
	uint32_t min_version;
	uint32_t cpu_id;
	uint32_t strings_size;
	uint32_t struct_size;
} DTBHeader;

typedef struct {
    uint32_t len;
    uint32_t nameoff;
} DTBProp;

void dtb_init(void);
