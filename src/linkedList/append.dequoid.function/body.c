node->next = 0;
node->data = data;
if(!(list->tail)) list->tail = list->head;
if(!(list->tail)){
	list->head = node;
	list->tail = list->head;
}
else
	list->tail->next = node;
list->tail = node;
return 0;
