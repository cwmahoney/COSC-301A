struct node {
    pid_t proc;
	char *cmd;
	int run;
    struct node *next; 
	struct node *last; //need it for removal without stupidity
};

void insert(char *cmd, pid_t proc_id, struct node **head);
void clear_list(struct node *curnode);
void killNode(struct node *curnode, struct node **head);
