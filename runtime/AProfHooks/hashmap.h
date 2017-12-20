//#define MAXNUM 33554432; //max number of hashmap elements 32M
//#define SLOTLENGTH 54018521  // a large prime use
#define SLOTLENGTH 8521  // a large prime use
// #define MAXNUM 39916801
#define MAP_MISS -3
#define MAP_FULL -2
#define MAP_EXCEED -1
#define MAP_SUCCESS 0

typedef struct _hashmapelem {
	char * key;  // indicate the address of the memory cell
	unsigned int value;   // indicate the latest access of the memory cell
	struct _hashmapelem * next; // linked to the next element
	int hash;
} hashmapelem;

typedef struct _hashmap {
	struct _hashmapelem ** elements; // a large prime as the length of the element slots
	unsigned long slotsize; // size of the taken slots
	unsigned long totalelem;
} hashmap;


hashmap * createhashmap();
void destroyhashmap(hashmap * hmap);
int hashmapremove(hashmap * hmap, char * key);
unsigned int keytoindex(char * key);
int hashmapput(hashmap * hmap, char * key, unsigned int value);
int hashmapget(hashmap * hmap, char * key);
unsigned long int stringtoint(char * str);
int currentslotsize(hashmap * hmap);
int currentelemsize(hashmap * hmap);
