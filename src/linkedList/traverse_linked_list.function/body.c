int status;
struct linked_list *cycle_detector;
char counter;
if(!head) return 0;
cycle_detector = 0;
counter = 0;
while(head){
	if(cycle_detector == head) return 0;
	status = (*visitor)(head->data, context, head);
	if(status) return status;
	if(!cycle_detector) cycle_detector = head;
	head = head->next;
	if(counter) cycle_detector = cycle_detector->next;
	counter = !counter;
}
return 0;
