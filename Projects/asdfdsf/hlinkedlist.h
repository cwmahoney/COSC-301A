struct node{
 	char *value;
    struct node *next, *prev; 
};

void insert(char *value, struct node **head);
void clear_list(struct node *curnode);
void killNode(struct node *curnode, struct node **head);
