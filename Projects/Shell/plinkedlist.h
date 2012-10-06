struct node {
    pid_t proc;
	char *cmd;
	int run;
    struct node *next; 
	struct node *last; //need it for removal without stupidity
};

void insert(char *cmd, pid_t proc_id, struct node **head);
/*Wipes out the entire list after curnode*/
void clear_list(struct node *curnode);
/*removes a node from the ll starting at head*/
void killNode(struct node *curnode, struct node **head);
