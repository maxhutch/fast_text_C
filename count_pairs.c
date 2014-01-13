#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FSIZE 10000000
#define HASH_SIZE 11
#define MAX_LEN 32
#define NLINE 135593
//#define NLINE 100000
#define START_SIZE 4

typedef struct node {
  char* values; 
  struct node* next;
  char* key;
  int nval;
  int lval;
} node;

static node** hash_head;

int init_hash(unsigned long size){
  unsigned long i;
  hash_head = (node**) malloc((1<<HASH_SIZE) * sizeof(node));
  for (i = 0; i < 1<<HASH_SIZE; i++){
    hash_head[i] = NULL;
  } 
  return EXIT_SUCCESS;
}

unsigned int hash_code(char* key){
  unsigned int hash = 5381;
  int c;

  while (c = *key++)
    hash = ((hash << 4) + hash) + c; 
  return hash & ((1<<HASH_SIZE)-1);
}

static int hash_counter = 0;
static int node_counter = 0;

int add(char* key, char* val){
  unsigned int code = hash_code(key);
  node* ptr = hash_head[code];
  if (ptr == NULL){
    ptr = (node*) malloc(sizeof(node));
    ptr->next = NULL;
    ptr->key = (char*) malloc((strlen(key)+1) * sizeof(char));
    strcpy(ptr->key, key);
    ptr->lval = START_SIZE;
    ptr->values = (char*) malloc(ptr->lval * MAX_LEN * sizeof(char));
    strcpy(ptr->values, val);
    ptr->nval = 1;
    hash_head[code] = ptr;
    hash_counter ++; node_counter ++;
    return 1;
  }

  node* prev;
  while (ptr != NULL && strcmp(ptr->key, key) != 0){
    prev = ptr;
    ptr = ptr->next;
  }

  if (ptr == NULL){
    ptr = (node*) malloc(sizeof(node));
    ptr->next = NULL;
    ptr->key = (char*) malloc((strlen(key)+1) * sizeof(char));
    strcpy(ptr->key, key);
    ptr->lval = START_SIZE;
    ptr->values = (char*) malloc(ptr->lval * MAX_LEN * sizeof(char));
    strcpy(ptr->values, val);
    ptr->nval = 1;
    prev->next = ptr;
    node_counter++;
    return 1;
  }

  int i;
  for (i = 0; i < ptr->nval; i++){
    if (strcmp(ptr->values + i*MAX_LEN, val) == 0){
      return 0;
    }
  }    
  if (ptr->lval == ptr->nval){
    ptr->lval *= 2;
    char* tmp = (char*) realloc(ptr->values, ptr->lval * MAX_LEN * sizeof(char));
    if (!tmp) fprintf(stderr, "Realloc failed \n");
    ptr->values = tmp;
  }
  strcpy(ptr->values+ptr->nval * MAX_LEN, val);
  ptr->nval ++;
  return 1;
}

int main(int argc, char* argv[]){

  /* Pick file */
  FILE* fp = fopen(argv[1], "r");
 
  /* Make a huge buffer */ 
  char* buffer = (char*) malloc((FSIZE+1) * sizeof(char));
  char* ptr;
  char key[MAX_LEN], val[MAX_LEN];

  /* Read into that huge buffer */
  fread(buffer, sizeof(char), FSIZE, fp);

  /* Setup the hash table */
  init_hash(HASH_SIZE); 

  int i, j, counter = 0;
  ptr = buffer;

  int res;
  /* This loops over input lines */
  for (i = 0; i < NLINE; i++){
    j = 0;
    /* Loop for the comma separator */
    while (ptr[j] != ','){
      key[j] = ptr[j];
      j = j + 1;
    }
    key[j] = '\0';
    ptr += j+1;
    j = 0;
    /* Loop for the newline separator */
    while (ptr[j] != '\n'){
      val[j] = ptr[j];
      j = j + 1;
    }
    /* Check size vs our static sized values */
    if (j > MAX_LEN-1){
      ptr[j] = '\0';
      fprintf(stderr, "Val %s is too large \n", ptr);
      break;
    }
    val[j] = '\0';
    ptr += j+1;
    /* Add it */
    counter += add(key, val);
  } 

  /* Print some info */
  fprintf(stdout, "Used  %d unique hashes \n", hash_counter);
  fprintf(stdout, "Used  %d unique keys \n", node_counter);
  fprintf(stdout, "Added %d unique pairs \n", counter);

  return EXIT_SUCCESS;
}
