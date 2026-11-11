#include <string.h> // For strlen(), strcmp(), strcpy()
#include "markov_chain.h"

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

#define EMPTY -1
#define BOARD_SIZE 100
#define MAX_GENERATION_LENGTH 60

#define DICE_MAX 6
#define NUM_OF_TRANSITIONS 20
#define ARG_NUM 3
#define INVALID_ARG_NUM "Usage: The program receives 3 or 4 arguments only.\n"

/**
 * represents the transitions by ladders and snakes in the game
 * each tuple (x,y) represents a ladder from x to if x<y or a snake otherwise
 */
const int transitions[][2] = {{13, 4},
                              {85, 17},
                              {95, 67},
                              {97, 58},
                              {66, 89},
                              {87, 31},
                              {57, 83},
                              {91, 25},
                              {28, 50},
                              {35, 11},
                              {8,  30},
                              {41, 62},
                              {81, 43},
                              {69, 32},
                              {20, 39},
                              {33, 70},
                              {79, 99},
                              {23, 76},
                              {15, 47},
                              {61, 14}};

/**
 * struct represents a Cell in the game board
 */
typedef struct Cell
{
    int number; // Cell number 1-100
    int ladder_to;  // ladder_to represents the jump of the
    // ladder in case there is one from this square
    int snake_to;  // snake_to represents the jump of the snake
    // in case there is one from this square
    //both ladder_to and snake_to should be -1 if the Cell doesn't have them
} Cell;

/** Error handler **/
static int handle_error (char *error_msg, MarkovChain **database)
{
  printf ("%s", error_msg);
  if (database != NULL)
  {
    free_markov_chain (database);
  }
  return EXIT_FAILURE;
}

static int create_board (Cell *cells[BOARD_SIZE])
{
  for (int i = 0; i < BOARD_SIZE; i++)
  {
    cells[i] = malloc (sizeof (Cell));
    if (cells[i] == NULL)
    {
      for (int j = 0; j < i; j++)
      {
        free (cells[j]);
      }
      handle_error (ALLOCATION_ERROR_MASSAGE, NULL);
      return EXIT_FAILURE;
    }
    *(cells[i]) = (Cell) {i + 1, EMPTY, EMPTY};
  }

  for (int i = 0; i < NUM_OF_TRANSITIONS; i++)
  {
    int from = transitions[i][0];
    int to = transitions[i][1];
    if (from < to)
    {
      cells[from - 1]->ladder_to = to;
    }
    else
    {
      cells[from - 1]->snake_to = to;
    }
  }
  return EXIT_SUCCESS;
}

/**
 * fills database
 * @param markov_chain
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int fill_database (MarkovChain *markov_chain)
{
  Cell *cells[BOARD_SIZE];
  if (create_board (cells) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }
  MarkovNode *from_node = NULL, *to_node = NULL;
  size_t index_to;
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    add_to_database (markov_chain, cells[i]);
  }

  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    from_node = get_node_from_database (markov_chain, cells[i])->data;

    if (cells[i]->snake_to != EMPTY || cells[i]->ladder_to != EMPTY)
    {
      index_to = MAX(cells[i]->snake_to, cells[i]->ladder_to) - 1;
      to_node = get_node_from_database (markov_chain, cells[index_to])
          ->data;
      add_node_to_counter_list (from_node, to_node, markov_chain);
    }
    else
    {
      for (int j = 1; j <= DICE_MAX; j++)
      {
        index_to = ((Cell *) (from_node->data))->number + j - 1;
        if (index_to >= BOARD_SIZE)
        {
          break;
        }
        to_node = get_node_from_database (markov_chain, cells[index_to])
            ->data;
        add_node_to_counter_list (from_node, to_node, markov_chain);
      }
    }
  }
  // free temp arr
  for (size_t i = 0; i < BOARD_SIZE; i++)
  {
    free (cells[i]);
  }
  return EXIT_SUCCESS;
}

static bool (is_last_cell) (void *my_data)
{
  Cell *my_cell = (Cell *) my_data;
  return my_cell->number == BOARD_SIZE;
}

static void print_game (void *data)
{
  Cell *my_cell = (Cell *) data;
  if (is_last_cell (my_cell))
  {
    printf ("[%d]", my_cell->number);
  }
  else if (my_cell->ladder_to != EMPTY)
  {
    printf ("[%d]-ladder to %d -> ", my_cell->number, my_cell->ladder_to);
  }
  else if (my_cell->snake_to != EMPTY)
  {
    printf ("[%d]-snake to %d -> ", my_cell->number, my_cell->snake_to);
  }
  else
  {
    printf ("[%d] -> ", my_cell->number);
  }
}

static int comp_cell (void *first, void *last)
{
  Cell *my_first = (Cell *) first;
  Cell *my_last = (Cell *) last;
  return ((my_first->number) - (my_last->number));
}

static void *copy_cell (void *data)
{
  //todo: check validation.
  Cell *my_cell = malloc (sizeof (Cell));
  if (my_cell == NULL)
  {
    printf ("%s", ALLOCATION_ERROR_MASSAGE);
    return NULL;
  }
  memmove (my_cell, data, sizeof (Cell));
  return my_cell;
}

static void free_cell (void *data)
{
  if (data == NULL)
  {
    return;
  }
  free (data);
  data = NULL;
}

static void cell_markov_chin (MarkovChain *markov)
{
  markov->comp_func = comp_cell;
  markov->copy_func = copy_cell;
  markov->free_data = free_cell;
  markov->is_last = is_last_cell;
  markov->print_func = print_game;
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
}

/**
 * @param argc num of arguments
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int main (int argc, char *argv[])
{
  if (argc != ARG_NUM)
  {
    printf ("%s", INVALID_ARG_NUM);
    return EXIT_FAILURE;
  }
  unsigned int seed = strtol (argv[1], NULL, 10);
  srand (seed);
  int num_games_to_play = strtol (argv[2], NULL, 10);
  MarkovChain *my_markov_chain = malloc (sizeof (MarkovChain));
  if (my_markov_chain == NULL)
  {
    printf ("%s", ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }
  cell_markov_chin (my_markov_chain);
  my_markov_chain->database = NULL;
  if (fill_database (my_markov_chain) != 0)
  {
    free_markov_chain (&my_markov_chain);
    printf ("%s", ALLOCATION_ERROR_MASSAGE);
    return EXIT_FAILURE;
  }
  for (int i = 0; i < num_games_to_play; i++)
  {
    printf ("Random walk %d: ", i + 1);
    generate_random_sequence (my_markov_chain,
                              my_markov_chain->database->first->data, MAX_GENERATION_LENGTH);
  }
  free_markov_chain (&my_markov_chain);
  return EXIT_SUCCESS;
}
