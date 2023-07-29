/// Contains all the standard commands

#ifndef W_COMMAND_H
#define W_COMMAND_H

#include "interpreter.h"

W_COMMAND(w_cmd_echo); // echoes all given values
W_COMMAND(w_cmd_echoln); // same as above, but echoes a newline (U+000A) after

W_COMMAND(w_cmd_read); // reads all of stdin
W_COMMAND(w_cmd_readln); // echoes a prompt, and reads a single line

// arithmetic operations

W_COMMAND(w_cmd_add);
W_COMMAND(w_cmd_sub);
W_COMMAND(w_cmd_mul);
W_COMMAND(w_cmd_div);
W_COMMAND(w_cmd_mod);

// state operations

W_COMMAND(w_cmd_set); // sets variables
W_COMMAND(w_cmd_let); // declares variables
W_COMMAND(w_cmd_swap); // swaps variables
W_COMMAND(w_cmd_del);

// boolean operations

W_COMMAND(w_cmd_equ);
W_COMMAND(w_cmd_neq);
W_COMMAND(w_cmd_lt);
W_COMMAND(w_cmd_lte);
W_COMMAND(w_cmd_gt);
W_COMMAND(w_cmd_gte);
W_COMMAND(w_cmd_or);
W_COMMAND(w_cmd_and);

// type conversions

W_COMMAND(w_cmd_int);
W_COMMAND(w_cmd_float);
W_COMMAND(w_cmd_string);

// control constructs

W_COMMAND(w_cmd_if);
W_COMMAND(w_cmd_break);
W_COMMAND(w_cmd_continue);
W_COMMAND(w_cmd_return);
W_COMMAND(w_cmd_while);
W_COMMAND(w_cmd_do); // does a block
W_COMMAND(w_cmd_for);

// data types

W_COMMAND(w_cmd_list);
W_COMMAND(w_cmd_range); // takes 2 arguments: $start, $end. returns a range containing [$start, $end)
W_COMMAND(w_cmd_map);

W_COMMAND(w_cmd_refcount); // gets the refcount of a value. returns -1 if the given value does not have a refcount

// list operations

W_COMMAND(w_cmd_list_set_mut); // sets a value in an list
W_COMMAND(w_cmd_list_set);
W_COMMAND(w_cmd_list_push_mut); // pushes values to the end of lists
W_COMMAND(w_cmd_list_push);
W_COMMAND(w_cmd_list_unshift_mut); // pushes values to the start of lists 
W_COMMAND(w_cmd_list_unshift);
W_COMMAND(w_cmd_list_pop_mut); // pops a value from the end of a list
W_COMMAND(w_cmd_list_pop);
W_COMMAND(w_cmd_list_shift_mut); // pops a value from the start of a list
W_COMMAND(w_cmd_list_shift);
W_COMMAND(w_cmd_list_slice_mut); // slices a list
W_COMMAND(w_cmd_list_slice);
W_COMMAND(w_cmd_list_cat_mut); // concats lists
W_COMMAND(w_cmd_list_cat);
W_COMMAND(w_cmd_list_fill_mut); // fills a list
W_COMMAND(w_cmd_list_fill);
W_COMMAND(w_cmd_list_dup_mut); // duplicates a list
W_COMMAND(w_cmd_list_dup);
W_COMMAND(w_cmd_list_reverse_mut); // reverses a list
W_COMMAND(w_cmd_list_reverse);

W_COMMAND(w_cmd_new_list); // makes a list with N entries

// map operations

W_COMMAND(w_cmd_map_set_mut); // sets a value in a map
W_COMMAND(w_cmd_map_set);
W_COMMAND(w_cmd_map_del_mut);
W_COMMAND(w_cmd_map_del);

// string operations
W_COMMAND(w_cmd_string_set_mut);
W_COMMAND(w_cmd_string_set);
W_COMMAND(w_cmd_string_slice_mut);
W_COMMAND(w_cmd_string_slice);
W_COMMAND(w_cmd_string_dup_mut);
W_COMMAND(w_cmd_string_dup);
W_COMMAND(w_cmd_string_split);
W_COMMAND(w_cmd_string_reverse_mut);
W_COMMAND(w_cmd_string_reverse);
W_COMMAND(w_cmd_string_split);

W_COMMAND(w_cmd_clone); // clones a list or a map

W_COMMAND(w_cmd_cmd); // creates a command

#endif
