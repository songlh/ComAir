#define INIEXPOSIZE 1024

typedef struct _shadowstackelem {
	char * funcname; // function name of current activation
	unsigned int ts; // time stamp of 
	unsigned int rms;
	unsigned int cost;
} shadowstackelem;

typedef struct _shadowstack {
	struct _shadowstackelem * stackelements;
	unsigned long top; // ****extra attention, stackelemets[top-1] is the top element. 
	unsigned long stacksize; // current size of the stack
	unsigned long elementsnum; // the size for the next expand
} shadowstack;

int pushstackelem(shadowstack * psstack, char * funcname, unsigned int ts);
int popstackelem(shadowstack * psstack);
int emptystack(shadowstack * psstack);
struct _shadowstackelem * topstackelem(shadowstack * psstack);
shadowstack * createshadowstack();
int expandstack(shadowstack * psstack);

void call(shadowstack * psstack, char * funcname);
int callreturn();
unsigned int getcost();
void collect(shadowstack * psstack); //cost collection to be finished
void read(unsigned long startaddr, unsigned int length);
void write(unsigned long startaddr, unsigned int length);
