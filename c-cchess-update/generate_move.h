#ifndef WSUN_CCHESS_GENERATE_MOVE_H
#define WSUN_CCHESS_GENERATE_MOVE_H

struct piece;
int legal_move_king(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_advisor(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_bishop(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_knight(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_rook(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_cannon(struct piece* piece, struct piece* const* pieces, int dest);
int legal_move_pawn(struct piece* piece, struct piece* const* pieces, int dest);

int generate_moves_king(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_advisor(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_bishop(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_knight(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_rook(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_cannon(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_moves_pawn(struct piece* piece, struct piece* const* pieces, int* mvs);

int generate_capture_moves_king(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_advisor(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_bishop(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_knight(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_rook(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_cannon(struct piece* piece, struct piece* const* pieces, int* mvs);
int generate_capture_moves_pawn(struct piece* piece, struct piece* const* pieces, int* mvs);

#endif

