#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "step_recorder.h"

const int INIT_HISTORY_STEPS_RECORD_SIZE = (2 << 10);

struct step* step_create()
{
	struct step* step = (struct step*)malloc(sizeof(struct step));
	memset(step, 0, sizeof(struct step));
	return step;
}

void step_release(struct step* step)
{
	free(step);
}

struct step_recorder* step_recorder_create(int capacity)
{
  struct step_recorder* recorder =
    (struct step_recorder*)malloc(sizeof(struct step_recorder));
  recorder->steps = (struct step**)malloc(sizeof(struct step*) * capacity);
  for (int i = 0; i < capacity; ++i) recorder->steps[i] = step_create();
  recorder->size = 0;
  recorder->capacity = capacity;
  return recorder;
}

void step_recorder_add_step(struct step_recorder* recorder, const struct step step)
{
  if (recorder->size == recorder->capacity)
  {
    printf("expand history step recorder capacity\n");
		int new_capacity = (recorder->capacity << 1);
		struct step** new_steps =
			(struct step**)malloc(sizeof(struct Step*) * new_capacity);
		int i = 0;
		memcpy(new_steps, recorder->steps, recorder->capacity);
    for (int i = recorder->capacity; i < new_capacity; ++i)
      new_steps[i] = step_create();
    recorder->capacity = new_capacity;
  }
  struct step* cur_step = recorder->steps[recorder->size++];
  cur_step->mv = step.mv;
  cur_step->in_check = step.in_check;
  cur_step->end_piece = step.end_piece;
  cur_step->zobrist_key = step.zobrist_key;
}

int step_recorder_empty(struct step_recorder* recorder) {
  return recorder->size == 0;
}

struct step* step_recorder_back_step(struct step_recorder* recorder)
{
  assert(recorder->size > 0);
  return recorder->steps[--recorder->size];
}

void step_recorder_release(struct step_recorder* recorder)
{
  for (int i = 0; i < recorder->capacity; ++i)
    step_release(recorder->steps[i]);
  free(recorder->steps);
}

