#include <arpa/inet.h>

struct node{
 	int socket;
	struct sockaddr_in client_address;
    struct node *next;
};

void insert(int socket, struct sockaddr_in client_address, struct node **head, struct node **tail);
//void clear_list(struct node *curnode);
void killHead(struct node **head_ptr, struct node **tail_ptr);
