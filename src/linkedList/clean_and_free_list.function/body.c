int status;
struct linked_list *ptr;
status = traverse_linked_list(head, cleaner, context);
if(status) return status;
while(head){
	head->data = 0;
	ptr = head->next;
	head->next = 0;
	free(head);
	head = ptr;
}
return 0;
