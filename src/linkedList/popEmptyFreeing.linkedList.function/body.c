struct linked_list *node = 0;
if(!xs) return 1;
while(*xs && !((*xs)->data)){
	node = (*xs)->next;
	(*xs)->next = 0;
	free(*xs);
	*xs = node;
}
xs = 0;
return 0;
