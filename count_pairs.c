#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

// file buffer size
#define FSIZE 10000000
// log_2(word hash size)
#define WHASH_SIZE 12
// key-value hash size
#define KVHASH_SIZE 12000
// maximum length of key-value (for parsing)
#define MAX_LEN 100
// number of lines to read
#define NLINE 135593
//#define NLINE 100000
// starting size of value array in kv hash
#define START_SIZE 2

/* LL nodes for hash from char* -> unsigned short */
typedef struct word_node {
  char* key;
  unsigned short val;  
  struct word_node* next; // LL pointer
} word_node;

/* Array nodes for hash from unsigned short -> [unsigned short] */
typedef struct kv_node {
  unsigned short* values; // array of values
  unsigned short lvalues; // capacity of array
} kv_node;

static word_node* word_hash;
static kv_node* kv_hash;

/* alloc and init the tops of the hashes */
int init_hash(unsigned long size){
  unsigned long i;
  word_hash = (word_node*) malloc((1<<WHASH_SIZE) * sizeof(word_node));
  kv_hash = (kv_node*) malloc(KVHASH_SIZE * sizeof(kv_node));

  for (i = 0; i < 1<<WHASH_SIZE; i++){
    word_hash[i].key = NULL;
  } 

  for (i = 0; i < KVHASH_SIZE; i++){
    kv_hash[i].values = NULL;
  } 

  return EXIT_SUCCESS;
}

// from http://www.cse.yorku.ca/~oz/hash.html
unsigned int hash_code(char* key){
  unsigned short hash = 5381;
  char c;

  while (c = *key++)
    hash = ((hash << 4) + hash) + c; 
  return hash & ((1<<WHASH_SIZE)-1);
}

static int hash_counter = 0;
static int node_counter = 0;
static double hash_time = 0;

/* This will densely assign values to the keys */
static unsigned short running_index = 0;

unsigned short trans(char* key){
//  hash_time -= MPI_Wtime();
  unsigned int code = hash_code(key);
//  hash_time += MPI_Wtime();
  word_node* ptr = word_hash + code;

  /* Check if this is the first key in bin */
  if (ptr->key == NULL){
    ptr->next = NULL;
    ptr->key = (char*) malloc((strlen(key)+1) * sizeof(char));
    strcpy(ptr->key, key);
    ptr->val = running_index;
    running_index++;
    hash_counter ++; node_counter ++;
    return ptr->val;
  }

  /* Walk the LL to the right key or the end */
  word_node* prev;
  while (ptr != NULL && strcmp(ptr->key, key) != 0){
    prev = ptr;
    ptr = ptr->next;
  }

  /* If we hit the end, add the key */
  if (ptr == NULL){
    ptr = (word_node*) malloc(sizeof(word_node));
    prev->next = ptr;
    ptr->next = NULL;
    ptr->key = (char*) malloc((strlen(key)+1) * sizeof(char));
    strcpy(ptr->key, key);
    ptr->val = running_index;
    running_index++;
    node_counter++;
    return ptr->val;
  }
  /* If we hit the key, return the value */
  return ptr->val;
}

/* Add (key,value) to kv_hash */
int add(unsigned short key, unsigned short val){
  /* key's are dense and unique! */
  kv_node* ptr = kv_hash + key;

  /* If this is the first value, allocate some stuff */
  if (ptr->values == NULL){
    ptr->lvalues = START_SIZE;
    ptr->values = (unsigned short*) malloc(ptr->lvalues * sizeof(unsigned short));
    ptr->values[0] = val;
    ptr->values[1] = -1; // marks end of array
    return 1;
  }

  /* Search for the right value */
  int i = 0;
  while (1){
    if (ptr->values[i] == val){ // value is there
      if (i > 0){ // swaps value earlier in the list so it is found faster in the future
        ptr->values[i] = ptr->values[i-1];
        ptr->values[i-1] = val;
      }
      return 0;
    }
    if (ptr->values[i] == (unsigned short) -1){ // hit the end
      ptr->values[i] = val; // add the value
      if (i+2 > ptr->lvalues){ // check if there is enough space
        ptr->lvalues *= 2; // double size on overflow
        unsigned short* tmp = realloc(ptr->values, ptr->lvalues * sizeof(unsigned short));
        if (!tmp) fprintf(stderr, "Realloc failed %d \n", ptr->lvalues);
        ptr->values = tmp;
      }
      ptr->values[i+1] = -1; // marks end of array
      return 1;
    } 
    i++;
  }
  return -1;
}

int main(int argc, char* argv[]){

  /* Pick file */
  FILE* fp = fopen(argv[1], "r");
 
  /* Make a huge buffer */ 
  char* buffer = (char*) malloc((FSIZE+1) * sizeof(char));
  char* ptr;
  char key[MAX_LEN], val[MAX_LEN];
  unsigned short tkey = -1, tval;

  /* Read into that huge buffer */
  double read_time = 0., parse_time = 0., trans_time = 0.;
// read_time = - MPI_Wtime();
  fread(buffer, sizeof(char), FSIZE, fp);
//  read_time += MPI_Wtime();
  /* Setup the hash table */
  init_hash(WHASH_SIZE); 

  int i, j, counter = 0;
  ptr = buffer;

  /* This loops over input lines */
  /* Bootstrap */
  for (i = 0; i < NLINE; i++){
//    parse_time -= MPI_Wtime();
    j = 0;
    /* Loop for the comma separator */
    if (tkey < (unsigned short) -1){ // need to read the key
      while (ptr[j] != ','){
        key[j] = ptr[j];
        j = j + 1;
      }
      key[j] = '\0';
    } else { // key is the old value
      while (ptr[j] != ','){
        j = j + 1;
      }
    }
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
//    parse_time += MPI_Wtime();
//    trans_time -= MPI_Wtime();
    /* Translate what we need */
    if (tkey < (unsigned short) -1)
      tkey = tval;
    else
      tkey = trans(key);
    tval = trans(val);
//    trans_time += MPI_Wtime();
    /* Add it */
    counter += add(tkey, tval);
  } 

  /* Print some info */
  fprintf(stdout, "Read  time: %f s \n", read_time);
  fprintf(stdout, "Parse time: %f s \n", parse_time);
  fprintf(stdout, "Trans time: %f s \n", trans_time);
  fprintf(stdout, "Hash  time: %f s \n", hash_time);
  fprintf(stdout, "Used  %d unique hashes \n", hash_counter);
  fprintf(stdout, "Used  %d unique keys \n", node_counter);
  fprintf(stdout, "Added %d unique pairs \n", counter);

  return EXIT_SUCCESS;
}
