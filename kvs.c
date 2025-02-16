#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "kvs.h"
#include "operations.h"
#include "string.h"

int hash(const char *key) {
  int firstLetter = tolower(key[0]);
  if (firstLetter >= 'a' && firstLetter <= 'z') {
    return firstLetter - 'a';
  } else if (firstLetter >= '0' && firstLetter <= '9') {
    return firstLetter - '0';
  }
  return -1; // Invalid index for non-alphabetic or number strings
}

/*AUXILIARY FUNCTIONS*/

void destroy_locks(HashTable *ht, int up_to_index) {
  for (int i = 0; i < up_to_index; i++) {
    pthread_rwlock_destroy(&ht->table[i].list_lock);
  }
}

/*END OF AUXILIARY FUNCTIONS*/

struct HashTable *create_hash_table() {
  // Allocate memory for the hash table
  HashTable *ht = malloc(sizeof(HashTable));
  if (!ht) {
    fprintf(stderr, "Failed to allocate memory for hash table\n");
    return NULL;
  }

  // Initialize each bucket's list and its lock
  for (int i = 0; i < TABLE_SIZE; i++) {
    ht->table[i].head = NULL; // Set the head pointer to NULL

    if (pthread_rwlock_init(&ht->table[i].list_lock, NULL) != 0) {
      destroy_locks(ht, i);
      free(ht);
      return NULL;
    }
  }

  // Initialize the global lock
  if (pthread_rwlock_init(&ht->global_lock, NULL) != 0) {
    destroy_locks(ht, TABLE_SIZE);
    free(ht);
    return NULL;
  }
  return ht; // Successfully created hash table
}

int write_pair(HashTable *ht, const char *key, const char *value) {
  int index = hash(key);
  KeyNode *keyNode = ht->table[index].head;
  // Search for the key node

  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) {
      free(keyNode->value);
      keyNode->value = strdup(value);
      return 0;
    }
    keyNode = keyNode->next; // Move to the next node
  }

  // Key not found, create a new key node
  keyNode = safe_malloc(sizeof(KeyNode));
  keyNode->key = strdup(key);            // Allocate memory for the key
  keyNode->value = strdup(value);        // Allocate memory for the value
  keyNode->next = ht->table[index].head; // Link to existing nodes
  ht->table[index].head =
      keyNode; // Place new key node at the start of the list
  return 0;
}

char *read_pair(HashTable *ht, const char *key) {
  int index = hash(key);
  KeyNode *keyNode = ht->table[index].head;
  char *value;

  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) {
      value = strdup(keyNode->value);
      return value; // Return copy of the value if found
    }
    keyNode = keyNode->next; // Move to the next node
  }
  return NULL; // Key not found
}

int delete_pair(HashTable *ht, const char *key) {
  int index = hash(key);
  List *list = &ht->table[index];
  KeyNode *keyNode = list->head;
  KeyNode *prevNode = NULL;

  while (keyNode != NULL) {
    if (strcmp(keyNode->key, key) == 0) {
      if (prevNode == NULL) {
        // Node to delete is the first node in the list
        ht->table[index].head =
            keyNode->next; // Update the table to point to the next node
      } else {
        // Node to delete is not the first; bypass it
        prevNode->next =
            keyNode->next; // Link the previous node to the next node
      }

      free(keyNode->key);
      free(keyNode->value);
      free(keyNode);
      return 0;
    }
    prevNode = keyNode;      // Move prevNode to current node
    keyNode = keyNode->next; // Move to the next node
  }

  return 1;
}

void free_table(HashTable *ht) {
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = ht->table[i].head;
    while (keyNode != NULL) {
      KeyNode *temp = keyNode;
      keyNode = keyNode->next;
      free(temp->key);
      free(temp->value);
      free(temp);
    }
    ht->table[i].head = NULL;
  }
  destroy_locks(ht, TABLE_SIZE);
  pthread_rwlock_destroy(&ht->global_lock);
  free(ht);
}