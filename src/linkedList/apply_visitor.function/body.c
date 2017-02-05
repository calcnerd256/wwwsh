visitor_t visitor;
memcpy(&visitor, fnptr_bytes, sizeof(visitor_t));
return (*visitor)(data, context, node);
