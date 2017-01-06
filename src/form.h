/* -*- indent-tabs-mode: t; tab-width: 2; c-basic-offset: 2; c-default-style: "stroustrup"; -*- */

int sampleForm_urlMatchesp(struct httpResource *res, struct extent *url);
int visit_header_getContentLength(struct linked_list *header, int *contentLength, struct linked_list *node);
int sampleForm_canRespondp(struct httpResource *res, struct incomingHttpRequest *req);
int sampleForm_respond_GET(struct httpResource *res, struct incomingHttpRequest *req);
int sampleForm_respond_POST(struct httpResource *res, struct incomingHttpRequest *req);
int sampleForm_respond(struct httpResource *res, struct incomingHttpRequest *req);
