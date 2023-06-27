#ifndef WSUN_CCHESS_STEP_RECORDER_H
#define WSUN_CCHESS_STEP_RECORDER_H

#include <inttypes.h>

typedef struct step
{
	int mv;
	int in_check;
	struct piece* end_piece;
	uint32_t zobrist_key;
} step;

typedef struct step_recorder {
  struct step** steps;
	int size;
	int capacity;
} step_recorder;

struct step_recorder* step_recorder_create(int capacity);
void step_recorder_add_step(struct step_recorder* recorder, const struct step step);
struct step* step_recorder_back_step(struct step_recorder* recorder);
int step_recorder_empty(struct step_recorder* recorder);
void step_recorder_release(struct step_recorder* recorder);

#endif
