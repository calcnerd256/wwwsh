/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

struct chunkStream{
	struct dequoid chunk_list;
	struct mempool *pool;
	struct linked_list *cursor_chunk;
	size_t cursor_chunk_offset;
};
