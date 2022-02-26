#include "board.h"
#include "search_engine.h"
#include <ctype.h>
#include <assert.h>

namespace wsun
{
namespace cchess
{

static int get_piece_type(char c)
{
	switch(c)
	{
		case 'K': return PIECE_TYPE_KING;
		case 'A': return PIECE_TYPE_ADVISOR;
		case 'B': return PIECE_TYPE_BISHOP;
		case 'N': return PIECE_TYPE_KNIGHT;
		case 'R': return PIECE_TYPE_ROOK;
		case 'C': return PIECE_TYPE_CANNON;
		case 'P': return PIECE_TYPE_PAWN;
		default: return -1;
	}
	return -1;
}

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

void Board::initHistoryStepRecords()
{
	history_step_records_capacity = INIT_HISTORY_STEPS_RECORD_SIZE;
	history_step_records = 
		(struct step**)malloc(sizeof(struct step*) * history_step_records_capacity);
	for (int i = 0; i < history_step_records_capacity; ++i)
	{
		history_step_records[i] = step_create();
	}
	history_step_records_size = 0;
}

void Board::releaseHistoryStepRecords()
{
	for (int i = 0; i < history_step_records_capacity; ++i)
	{
		step_release(history_step_records[i]);
	}
	free(history_step_records);
}

Board* Board::getExchangeSideBoard() const
{
	Board* b = new Board;
	b->resetData();
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		Piece* p = pieces_[i];
		if (p && p->show())
		{
			b->addPieceToBoard(p->type(), 1 - p->sidePlayer()->side(), 254 - p->pos());
		}
	}
	b->currentSidePlayer_ = 
		(currentSidePlayer_->side() == SIDE_TYPE_RED ? b->blackPlayer_ : b->redPlayer_);

	return b;
}

Board* Board::getMirrorBoard() const
{
	Board* b = new Board;
	b->resetData();
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		Piece* p = pieces_[i];
		if (p && p->show())
		{
			b->addPieceToBoard(p->type(), p->sidePlayer()->side(), mirror_pos(p->pos()));
		}
	}
	b->currentSidePlayer_ = 
		(currentSidePlayer_->side() == SIDE_TYPE_RED ? b->redPlayer_ : b->blackPlayer_);

	return b;
}

Zobrist Board::getMirrorZobrist() const
{
	ZobristHelper zh;
	for (int i = 0; i < 256; ++i)
	{
		if (!in_board(i)) continue;
		Piece* piece = pieces_[i];
		if (piece && piece->show())
			zh.updateByChangePiece(piece->sidePlayer()->side(), 
														 piece->type(), 
														 mirror_pos(piece->pos()));
	}
	int side = currentSidePlayer_->side();
	if (side == SIDE_TYPE_BLACK)
	{
		zh.updateByChangeSide();
	}
	return zh.getZobrist();
}

void Board::addPieceToBoard(int type, int side, int pos)
{
	Player* sidePlayer = 
		(side == SIDE_TYPE_RED ? redPlayer_ : blackPlayer_);
	Piece* piece = new Piece(type, sidePlayer, pos);
	pieces_[pos] = piece;
	sidePlayer->addPiece(piece);
	zobristHelper_.updateByChangePiece(side, type, pos);
}

void Board::resetData()
{
	redPlayer_->reset();
	blackPlayer_->reset();
	currentSidePlayer_ = redPlayer_;
	initPieceArray();
	history_step_records_size = 0;
	zobristHelper_.reset();
}

// 用fen串信息来初始化局面
int Board::initFromFen(const char* fen)
{
	int row = 0;
	int col = 0;

	int i = 0;
	while (!isspace(fen[i]))
	{
		char c = fen[i];
		if (isdigit(c))
		{
			col += (int)(c - '0');
		}
		else if (isalpha(c))
		{
			int side = SIDE_TYPE_RED;
			if (islower(c))
			{
				side = SIDE_TYPE_BLACK;
			}
			int type = get_piece_type(toupper(c));

			addPieceToBoard(type, side, convert_to_pos(row, col));
			++col;
		}
		else if (c == '/')
		{
			++row;
			col = 0;
		}
		++i;
	}

	if (fen[++i] == 'b')
		return SIDE_TYPE_BLACK;
	else
		return SIDE_TYPE_RED;
}

// 根据fen串信息重置局面
void Board::resetFromFen(const char* fen)
{
	resetData();

	int side = initFromFen(fen);
	currentSidePlayer_ = 
		(side == SIDE_TYPE_RED ? redPlayer_ : blackPlayer_);

	if (side == SIDE_TYPE_BLACK)
	{
		zobristHelper_.updateByChangeSide();
	}
}

// 重置到最初局面
void Board::reset()
{
	resetData();
	initFromFen(INIT_FEN_STRING);
}

// 根据当前局面输出fen格式的局面信息
std::string Board::toFen()
{
	char fenBuffer[100] = {0};
	char* fen = fenBuffer;
	for (int row = 0; row < 10; ++row)
	{
		int number = 0;
		for (int col = 0; col < 9; ++col)
		{
			int pos = convert_to_pos(row, col);
			if (pieces_[pos] && pieces_[pos]->show())
			{
				if (number > 0)
				{
					*fen = (char)('0' + number);
					++fen;
					number = 0;
				}
				int side = pieces_[pos]->sidePlayer()->side();
				*fen = fen_piece_char[side][pieces_[pos]->type()];
				++fen;
			}
			else 
			{
				++number;
			}
		}
		if (number > 0)
		{
			*fen = (char)('0' + number);
			++fen;
		}

		*fen = '/';
		++fen;
	}

	--fen;
	*fen = ' ';
	++fen;
	*fen = (currentSidePlayer_->side() == SIDE_TYPE_RED ? 'w' : 'b');

	return std::string(fenBuffer);
}

bool Board::willKillKing(Player* player)
{
	Player* oppPlayer = getOpponentPlayerByPlayer(player);
	Piece** pieces = oppPlayer->pieces();
	Piece* kingPiece = player->kingPiece();

	for (int i = 0; i < oppPlayer->piecesNum(); ++i)
	{
		if (pieces[i]->show() && legalMovePiece(pieces[i], kingPiece->pos()))
		{
			return true;
		}
	}
	return false;
}

// 生成某棋子所有的走法(注意：不存在走完之后依然被对方将军),
// capatured: 是否只生成吃子走法
int Board::generateMoves(Piece* piece, int* mvs, bool capatured)
{
	// 必须保证棋子在棋盘上
	if (!piece || !piece->show()) return 0;

	int nums = 0;
	if (piece->type() == PIECE_TYPE_KING)
	{
		// 九宫内的走法
		for (int i = 0; i < 4; ++i)
		{
			int dest = piece->pos() + array_king_delta[i];
			if (!in_fort(dest)) continue;

			if (!capatured)
			{
				if (!pieces_[dest])
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
			}
			
			if (piece->oppSide(pieces_[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_ADVISOR)
	{
		for (int i = 0; i < 4; ++i)
		{
			int dest = piece->pos() + array_advisor_delta[i];
			if (!in_fort(dest)) continue;

			if (!capatured)
			{
				if (!pieces_[dest])
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
			}
			
			if (piece->oppSide(pieces_[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_BISHOP)
	{
		for (int i = 0; i < 4; ++i)
		{
			int dest = piece->pos() + array_advisor_delta[i];

			if (!in_board(dest) || 
					!same_half(piece->pos(), dest) || 
					pieces_[dest])
			{
				continue;
			}

			dest += array_advisor_delta[i];
			if (!capatured)
			{
				if (!pieces_[dest])
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
			}
			
			if (piece->oppSide(pieces_[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_KNIGHT)
	{
		for (int i = 0; i < 4; ++i)
		{
			int dest = piece->pos() + array_king_delta[i];
			if (pieces_[dest]) continue;

			for (int j = 0; j < 2; ++j)
			{
				dest = piece->pos() + array_knight_delta[i][j];
				if (!in_board(dest)) continue;

				if (!capatured)
				{
					if (!pieces_[dest])
					{
						mvs[nums++] = get_move(piece->pos(), dest);
					}
				}
				
				if (piece->oppSide(pieces_[dest]))
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_ROOK)
	{
		for (int i = 0; i < 4; ++i)
		{
			int nDelta = array_king_delta[i];
			int dest = piece->pos() + nDelta;
			while (in_board(dest))
			{
				if (pieces_[dest])
				{
					if (piece->oppSide(pieces_[dest]))
					{
						mvs[nums++] = get_move(piece->pos(), dest);
					}
					break;
				}
				if (!capatured)
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
				dest += nDelta;
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_CANNON)
	{
		for (int i = 0; i < 4; ++i)
		{
			int nDelta = array_king_delta[i];
			int dest = piece->pos() + nDelta;
			while (in_board(dest))
			{
				if (!pieces_[dest])
				{
					if (!capatured)
					{
						mvs[nums++] = get_move(piece->pos(), dest);
					}
				}
				else break;

				dest += nDelta;
			}
			dest += nDelta;
			while (in_board(dest))
			{
				if (!pieces_[dest]) dest += nDelta;
				else
				{
					if (piece->oppSide(pieces_[dest]))
					{
						mvs[nums++] = get_move(piece->pos(), dest);
					}
					break;
				}
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_PAWN)
	{
		int dest = piece->forwardStep();
		if (in_board(dest))
		{
			// capatured move
			if (piece->oppSide(pieces_[dest]))
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}

			if (!capatured && !pieces_[dest])
			{
				mvs[nums++] = get_move(piece->pos(), dest);
			}
		}

		// 过河兵
		if (!same_half(piece->sidePlayer()->kingPiece()->pos(), piece->pos()))
		{
			for (int nDelta = -1; nDelta <= 1; nDelta += 2)
			{
				dest = piece->pos() + nDelta;

				if (!in_board(dest)) continue;

				// capatured move
				if (piece->oppSide(pieces_[dest]))
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}

				if (!capatured && !pieces_[dest])
				{
					mvs[nums++] = get_move(piece->pos(), dest);
				}
			}
		}
	}
	return nums;
}

// 生成所有棋子所有的走法 capatured: 是否只生成吃子走法
int Board::generateAllMovesNoncheck(int* mvs, bool capatured)
{
	int nums = 0;
	int n = 0;
	int temp[MAX_GENERATE_MOVES];
	Piece** pieces = currentSidePlayer_->pieces();
	for (int i = 0; i < 16; ++i)
	{
		n += generateMoves(pieces[i], &temp[n], capatured);
	}
	for (int i = 0; i < n; ++i)
	{
		makeMove(temp[i]);
		if (!willKillSelfKing())
		{
			mvs[nums++] = temp[i];
		}
		undoMove();
	}

	return nums;
}

int Board::generateAllMoves(int* mvs, bool capatured)
{
	int nums = 0;
	Piece** pieces = currentSidePlayer_->pieces();
	for (int i = 0; i < 16; ++i)
	{
		nums += generateMoves(pieces[i], &mvs[nums], capatured);
	}
	return nums;
}

void Board::addPiece(Piece* piece, int pos)
{
	if (!piece) return ;
	piece->setShow(true);
	piece->setPos(pos);
	pieces_[pos] = piece;
	piece->sidePlayer()->addPieceValue(piece);

	int side = piece->sidePlayer()->side();
	zobristHelper_.updateByChangePiece(side, piece->type(), pos);
}

Piece* Board::delPiece(int pos)
{
	Piece* piece = pieces_[pos];
	if (!piece) return NULL;

	piece->setShow(false);
	piece->sidePlayer()->delPieceValue(piece);
	pieces_[pos] = NULL;

	int side = piece->sidePlayer()->side();
	zobristHelper_.updateByChangePiece(side, piece->type(), pos);

	return piece;
}

bool Board::legalMovePiece(Piece* piece, int dest)
{
	if (piece->type() == PIECE_TYPE_KING)
	{
		// 判断在九宫的走法是否合理
		if(in_fort(dest) && array_legal_span[dest - piece->pos() + 256] == 1)
			return 1;
	}

	if (piece->type() == PIECE_TYPE_ADVISOR)
	{
		return in_fort(dest) && array_legal_span[dest - piece->pos() + 256] == 2;
	}
	else if (piece->type() == PIECE_TYPE_BISHOP)
	{
		return same_half(piece->pos(), dest) &&
					 array_legal_span[dest - piece->pos() + 256] == 3 &&
					 !pieces_[((piece->pos() + dest) >> 1)];
	}
	else if (piece->type() == PIECE_TYPE_KNIGHT)
	{
		return piece->pos() != piece->pos() + array_knight_pin[dest - piece->pos() + 256] && 
					 !pieces_[(piece->pos() + array_knight_pin[dest - piece->pos() + 256])];
	}
	else if (piece->type() == PIECE_TYPE_KING || piece->type() == PIECE_TYPE_ROOK || piece->type() == PIECE_TYPE_CANNON)
	{
		int offset = get_offset(piece->pos(), dest);
		if (offset == 0)
		{
			return 0;
		}
		else
		{
			int curPos = piece->pos() + offset;
			while (curPos != dest && !pieces_[curPos])
			{
				curPos += offset;
			}
			if (curPos == dest)
			{
				if (piece->type() == PIECE_TYPE_KING)
				{
					return pieces_[dest] && pieces_[dest]->type() == PIECE_TYPE_KING;
				}
				else if (piece->type() == PIECE_TYPE_ROOK)
				{
					return !piece->sameSide(pieces_[dest]);
				}
				else
				{
					return !pieces_[dest];
				}
			}
			else
			{
				if (piece->type() == PIECE_TYPE_KING || piece->type() == PIECE_TYPE_ROOK)
					return 0;

				curPos += offset;
				while (curPos != dest && !pieces_[curPos])
				{
					curPos += offset;
				}
				return curPos == dest;
			}
		}
	}
	else if (piece->type() == PIECE_TYPE_PAWN)
	{
		if (!same_half(piece->sidePlayer()->kingPiece()->pos(), piece->pos()) && 
				(piece->pos() + 1 == dest || piece->pos() - 1 == dest))
		{
			return 1;
		}
		return dest == piece->forwardStep();
	}

	return false;
}

bool Board::legalMove(int mv)
{
	bool legal = false;
	Piece* piece = pieces_[start_of_move(mv)];
	int dest = end_of_move(mv);
	if (piece && piece->show() && 
			piece->sidePlayer() == currentSidePlayer_ && 
			!piece->sameSide(pieces_[dest]) &&
			legalMovePiece(piece, dest))
	{
		makeMove(mv);
		if (!willKillSelfKing())
		{
			legal = true;
		}
		undoMove();
	}
	return legal;
}

// 真正走棋的动作
void Board::makeMove(int mv)
{
	int start = start_of_move(mv);
	int end = end_of_move(mv);

	uint32_t zkey = zobristHelper_.getZobrist().key_;

	Piece* endPiece = delPiece(end);
	Piece* retPiece = delPiece(start);
	addPiece(retPiece, end);

	int in_check = willKillOpponentKing();

	makeHistoryStep(mv, endPiece, in_check, zkey);
}

// 撤销上一步走棋
void Board::undoMove()
{
	if (history_step_records_size == 0) 
		return;

	struct step* step = history_step_records[--history_step_records_size];
	int mv = step->mv;
	int start = start_of_move(mv);
	int end = end_of_move(mv);

	Piece* retPiece = delPiece(end);
	addPiece(retPiece, start);
	addPiece(step->end_piece, end);
}

// 将每一步走法记录到历史表
void Board::makeHistoryStep(int mv, Piece* end_piece, int in_check, uint32_t zobrist_key)
{
	// 历史表容量已满则扩增
	if (history_step_records_size == history_step_records_capacity)
	{
		int new_capacity = (history_step_records_capacity << 1);
		struct step** steps_record =
			(struct step**)malloc(sizeof(struct Step*) * new_capacity);
		history_step_records_capacity = new_capacity;
		int i = 0;
		std::copy(history_step_records, 
							history_step_records + history_step_records_size, 
							steps_record);
		std::fill(steps_record + history_step_records_size,
							steps_record + history_step_records_capacity,
							step_create());
		free(history_step_records);
		history_step_records = steps_record;
	}

	struct step* step = history_step_records[history_step_records_size++];
	step->mv = mv;
	step->end_piece = end_piece;
	step->in_check = in_check;
	step->zobrist_key = zobrist_key;
}

// 检测重复局面
int Board::repetitionStatus(int recur)
{
	int self_side = 0;
	int perp_check = 1;
	int opponent_perp_check = 1;

	if (history_step_records_size == 0) 
		return 0;

	for(int i = history_step_records_size - 1; i >= 0; --i)
	{
		struct step* step = history_step_records[i];
		if (step->mv <= 0 || step->end_piece) break;

		if (self_side)
		{
			perp_check = perp_check && step->in_check;
			if (step->zobrist_key == zobristHelper_.getZobrist().key_)
			{
				if (--recur == 0)
				{
					return 1 + (perp_check ? 2 : 0) + (opponent_perp_check ? 4 : 0);
				}
			}
		}
		else 
		{
			opponent_perp_check = opponent_perp_check && step->in_check;
		}
		self_side = !self_side;
	}

	return 0;
}

int Board::prompt(int pos, int* mvs, int capatured)
{
	Piece* piece = pieces_[pos];
	int n = 0;
	int nums = 0;
	int temp[MAX_GENERATE_MOVES];
	if (piece && piece->sidePlayer() == currentSidePlayer_)
	{
		nums = generateMoves(piece, temp, capatured);
	}
	for (int i = 0; i < nums; ++i)
	{
		makeMove(temp[i]);
		if (!willKillSelfKing())
		{
			mvs[n++] = temp[i];
		}
		undoMove();
	}

	return n;
}

bool Board::noWayToMove()
{
	int mvs[MAX_GENERATE_MOVES];
	int n = generateAllMovesNoncheck(mvs);
	return n == 0;
}

} // namespace cchess
} // namespace wsun
