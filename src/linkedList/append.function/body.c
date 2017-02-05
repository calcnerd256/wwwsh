head = last_node(head);
if(!head) return 1;
head->next = malloc(sizeof(struct linked_list));
head = head->next;
head->data = data;
head->next = 0;
return 0;
