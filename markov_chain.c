#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "linked_list.c"
#include "markov_chain.h"
#include "linked_list.h"
#define MAX_ARG 5
#define MIN_ARG 4

/**
* Get random number between 0 and max_number [0, max_number).
* @param max_number maximal number to return (not including)
* @return Random number
*/
int get_random_number (int max_number)
{
  return rand () % max_number;
}

/**
* Check if the word ends with '.'.
* @param str the word we check.
* @return true if doesn't ends with dot, otherwise false.
*/
bool look_for_dot (char *str)
{ return str[strlen (str) - 1] != '.'; }

MarkovNode *get_first_random_node (MarkovChain *markov_chain)
{
  while (true)
  {
    int i = get_random_number (markov_chain->database->size);
    Node *cur_node = markov_chain->database->first;
    // get the first one:
    if (i == 0 && (markov_chain->is_last (cur_node->data->data) == false))
    {
      return cur_node->data;
    }
    for (int j = 0; j < i; j++)
    {
      cur_node = cur_node->next;
    }
    if ((markov_chain->is_last (cur_node->data->data)) == false)
    {
      return cur_node->data;
    }
  }
}

/**
 * Choose randomly the next state, depend on it's occurrence frequency.
 * @param state_struct_ptr MarkovNode to choose from
 * @return MarkovNode of the chosen state
 */
MarkovNode *get_next_random_node (MarkovNode *state_struct_ptr)
{
  int i = get_random_number (state_struct_ptr->counter_words);
  int j = 0;
  NextNodeCounter *cur_node;
  for (; 0 <= i;)
  {
    i -= state_struct_ptr->counter_list[j].frequency;
    cur_node = &state_struct_ptr->counter_list[j++];
  }
  return cur_node->markov_node;
}

/**
 * Receive markov_chain, generate and print random sentence out of it. The
 * sentence most have at least 2 words in it.
 * @param markov_chain
 * @param first_node markov_node to start with,
 *                   if NULL- choose a random markov_node
 * @param  max_length maximum length of chain to generate
 */
void generate_random_sequence (MarkovChain *markov_chain, MarkovNode
*first_node, int max_length)
{
  //check for valid node: todo: check if necessary.
  if (first_node == NULL)
  {
    first_node = get_first_random_node (markov_chain);
  }
  for (int i = 0; (i < max_length); i++)
  {
    if ((markov_chain->is_last (first_node->data) == false))
    {
      markov_chain->print_func (first_node->data);
      first_node = get_next_random_node (first_node);
    }
    else
    {
      markov_chain->print_func (first_node->data);
      break;
    }
  }
  printf ("\n");
}

/**
 * Free markov_chain and all of it's content from memory
 * @param markov_chain markov_chain to free
 */
void free_markov_chain (MarkovChain **ptr_chain)
{
  if (ptr_chain == NULL)
  {
    return;
  }
  Node *cur_node = (*ptr_chain)->database->first;
  while (cur_node != NULL)
  {
    free (cur_node->data->counter_list);
    //todo: check whether to use thu function "free data".
    (*ptr_chain)->free_data (cur_node->data->data);
    free (cur_node->data);
    Node *mid_node = cur_node->next;
    free (cur_node);
    cur_node = mid_node;
  }
  free ((*ptr_chain)->database);
  free (*ptr_chain);
}

/**
 * Add the second markov_node to the counter list of the first markov_node.
 * If already in list, update it's counter value.
 * @param first_node
 * @param second_node
 * @return success/failure: true if the process was successful, false if in
 * case of allocation error.
 */
bool add_node_to_counter_list (MarkovNode *first_node, MarkovNode
*second_node, MarkovChain *markov_chain)
{
  // in case word already in list.
  for (int i = 0; i < first_node->counter_list_size; i++)
  {
    if (markov_chain->comp_func ((first_node->counter_list[i]
        .markov_node->data), second_node->data) == 0)
    {
      first_node->counter_list[i].frequency += 1;
      first_node->counter_words += 1;
      return true;
    }
  }
  // in case word not in list.
  NextNodeCounter *tmp = realloc (first_node->counter_list,
                                  (first_node->counter_list_size + 1)
                                  * (sizeof (NextNodeCounter)));
  if (tmp == NULL)
  {
    return false;
  }
  first_node->counter_list = tmp;
  first_node->counter_list[first_node->counter_list_size].markov_node =
      second_node;
  first_node->counter_list[first_node->counter_list_size].frequency = 1;
  first_node->counter_list_size += 1;
  first_node->counter_words += 1;
  return true;
}

/**
* Check if data_ptr is in database. If so, return the markov_node
 * wrapping it in
 * the markov_chain, otherwise return NULL.
 * @param markov_chain the chain to look in its database
 * @param data_ptr the state to look for
 * @return Pointer to the Node wrapping given state, NULL if state not in
 * database.
 */
Node *get_node_from_database (MarkovChain *markov_chain, void *data_ptr)
{
  Node *cur_node = markov_chain->database->first;
  while (cur_node != NULL)
  {
    if (markov_chain->comp_func (cur_node->data->data, data_ptr) == 0)
    {
      return cur_node;
    }
    cur_node = cur_node->next;
  }
  return NULL;
}

/**
* If data_ptr in markov_chain, return it's node. Otherwise, create new
 * node, add to end of markov_chain's database and return it.
 * @param markov_chain the chain to look in its database
 * @param data_ptr the state to look for
 * @return node wrapping given data_ptr in given chain's database,
 * returns NULL in case of memory allocation failure.
 */
Node *add_to_database (MarkovChain *markov_chain, void *data_ptr)
{
  // in case database empty.
  if (markov_chain->database == NULL)
  {
    LinkedList *my_linked_list = malloc (sizeof (LinkedList));
    if (my_linked_list == NULL)
    {
      return NULL;
    }
    markov_chain->database = my_linked_list;
    markov_chain->database->first = NULL;
    markov_chain->database->last = NULL;
    markov_chain->database->size = 0;
  }
  // in case node already exist.
  Node *cur = get_node_from_database (markov_chain, data_ptr);
  if (cur != NULL)
  {
    return cur;
  }
  // in case node not exist.
  MarkovNode *new_markov_node = malloc (sizeof (MarkovNode));
  if (new_markov_node == NULL)
  {
    return NULL;
  }
  new_markov_node->data = markov_chain->copy_func (data_ptr);
  new_markov_node->counter_list = NULL;
  new_markov_node->counter_words = 0;
  new_markov_node->counter_list_size = 0;
  add (markov_chain->database, new_markov_node);
  return markov_chain->database->last;
}

int valid_path (FILE *path)
{
  if (path == NULL)
  {
    return 0;
  }
  return 1;
}

int valid_arg_num (int arg_num)
{
  if (arg_num == MIN_ARG || arg_num == MAX_ARG)
  {
    return 1;
  }
  return 0;
}