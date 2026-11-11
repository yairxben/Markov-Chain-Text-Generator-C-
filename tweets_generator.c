
#include "markov_chain.h"
//#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <stdbool.h>

#define ERROR "ERROR: "
#define INVALID_ARG_NUM "Usage: The program receives 3 or 4 arguments only.\n"
#define INVALID_FILE "The given file is invalid.\n"
#define MAX_LINE_LENGTH 1002
#define MAX_WORDS_TO_READ -1
#define MAX_TWEET_LEN 20
#define MAX_ARG 5


static void (print_char) (void *my_data){
  char * print_data = (char *)my_data;
  printf("%s ",print_data);
}

static int (comp_char) (void *first, void *second)
{
  return strcmp (first, second);
}

static void (free_char) (void * my_data){
  free(my_data);
}

static void *(copy_char) (void *my_data){
  int len = (int) strlen((char*)my_data);
  len = len +1;
  void *new_word = malloc(len);
  memmove(new_word,my_data,len);
  return new_word;
}

static bool (is_last_char) (void *my_data){
    char *my_word = (char *) my_data;
    return my_word[strlen (my_word) - 1] != '.';
}

void my_markov_chin(MarkovChain * markov){
  markov->comp_func = comp_char;
  markov->copy_func = copy_char;
  markov->free_data = free_char;
  markov->is_last = is_last_char;
  markov->print_func = print_char;

}



int fill_database (FILE *fp, int words_to_read, MarkovChain *markov_chain)
{
  char line[MAX_LINE_LENGTH];
  char *word;
  Node *cur_node = NULL;
  Node *prev_node = NULL;
  while (fgets (line, MAX_LINE_LENGTH + 1, fp))
  {
    word = strtok (line, " ");
    while (word != NULL && words_to_read != 0)
    {
      if (word[strlen (word) - 1] == '\n')
      {
        word[strlen (word) - 1] = '\0';
      }
      //copy word.
      //todo: free?
      char *new_word = malloc (strlen (word) + 1);
      if (new_word == NULL)
      {
        return 0;
      }
      strcpy (new_word, word);
      cur_node = add_to_database (markov_chain, new_word);
      if (cur_node == NULL)
      {
        return EXIT_FAILURE;
      }
      // first word.
      if (prev_node == NULL)
      {
        if ((look_for_dot (cur_node->data->data) == true))
        {
          prev_node = cur_node;
        }
      }
      else
      {
        if (add_node_to_counter_list (prev_node->data, cur_node->data, markov_chain)
            == false)
        {
          return EXIT_FAILURE;
        }
        if (look_for_dot (cur_node->data->data))
        {
          prev_node = cur_node;
        }
        else
        {
          prev_node = NULL;
        }
      }
      word = strtok (NULL, " ");
      words_to_read -= 1;
    }
  }
  return EXIT_SUCCESS;
}

int main (int argc, char *argv[])
{
  if (valid_arg_num (argc) == 0)
  {
    printf ("%s", INVALID_ARG_NUM);
    return EXIT_FAILURE;
  }
  FILE *file = fopen (argv[3], "r");
  if (valid_path (file) == 0)
  {
    printf ("%s%s", ERROR, INVALID_FILE);
    return EXIT_FAILURE;
  }
  unsigned int seed = strtol (argv[1], NULL, 10);
  srand (seed);
  int num_tweets_to_generate = strtol (argv[2], NULL, 10);
  MarkovChain *my_markov_chain = malloc (sizeof (MarkovChain));
  if (my_markov_chain == NULL)
  {
    fclose (file);
    printf ("%s", ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }
  my_markov_chain->database = NULL;
  int max_to_read = MAX_WORDS_TO_READ;
  if (argc == MAX_ARG)
  {
    max_to_read = strtol (argv[4], NULL, 10);
  }
  if (fill_database (file, max_to_read, my_markov_chain) != 0)
  {
    fclose (file);
    free_markov_chain (&my_markov_chain);
    printf ("%s", ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }
  for (int i = 0; i < num_tweets_to_generate; i++)
  {
    printf ("Tweet %d: ", i + 1);
    MarkovNode *first_word = get_first_random_node (my_markov_chain);
    generate_random_sequence (my_markov_chain,
                              first_word, MAX_TWEET_LEN);
  }
  fclose (file);
  free_markov_chain (&my_markov_chain);
  return EXIT_SUCCESS;
}