#ifndef KEY_VALUE_STORE_H
#define KEY_VALUE_STORE_H

#define TABLE_SIZE 26

#include <pthread.h>
#include <stddef.h>

typedef struct KeyNode {
  char *key;
  char *value;
  struct KeyNode *next;
} KeyNode;

typedef struct List {
  KeyNode *head;
  pthread_rwlock_t list_lock;
} List;

typedef struct HashTable {
  List table[TABLE_SIZE];
  pthread_rwlock_t global_lock;
} HashTable;

// Hash function based on key initial.
// @param key Lowercase alphabetical string.
// @return hash.
// NOTE: This is not an ideal hash function, but is useful for test purposes of
// the project
int hash(const char *key);

/// Destroys the locks associated with the hash table up to the given index.
/// @param ht Pointer to the hash table whose locks will be destroyed.
/// @param up_to_index The index up to which the locks should be destroyed.
/// This function will iterate over the hash table up to the specified index
/// and release any locks held by the table entries.
void destroy_locks(HashTable *ht, int up_to_index);

/// Creates a new event hash table.
/// @return Newly created hash table, NULL on failure
struct HashTable *create_hash_table();

/// Appends a new key value pair to the hash table.
/// @param ht Hash table to be modified.
/// @param key Key of the pair to be written.
/// @param value Value of the pair to be written.
/// @return 0 if the node was appended successfully, 1 otherwise.
int write_pair(HashTable *ht, const char *key, const char *value);

/// Deletes the value of given key.
/// @param ht Hash table to delete from.
/// @param key Key of the pair to be deleted.
/// @return 0 if the node was deleted successfully, 1 otherwise.
char *read_pair(HashTable *ht, const char *key);

/// Appends a new node to the list.
/// @param list Event list to be modified.
/// @param key Key of the pair to read.
/// @return 0 if the node was appended successfully, 1 otherwise.
int delete_pair(HashTable *ht, const char *key);

/// Frees the hashtable.
/// @param ht Hash table to be deleted.
void free_table(HashTable *ht);

#endif // KVS_H
