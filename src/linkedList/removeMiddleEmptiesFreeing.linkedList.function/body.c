struct linked_list *node = xs;
struct linked_list *middle;
while(node){
	while(node->next && !(node->next->data)){
		middle = node->next;
		node->next = middle->next;
		middle->next = 0;
		free(middle);
	}
	node = node->next;
}
xs = 0;
node = 0;
middle = 0;
return 0;
