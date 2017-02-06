/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

int point_extent_at_nice_string(struct extent *storage, char *bytes){
	storage->bytes = bytes;
	storage->len = strlen(bytes);
	return 0;
}
