char *result;
result = malloc(sizeof(visitor_t));
memcpy(result, &visitor, sizeof(visitor_t));
return result;
