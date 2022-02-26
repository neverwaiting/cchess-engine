#ifndef __WSUN_CCHESS_BOARD_H__
#define __WSUN_CCHESS_BOARD_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "zobrist_helper.h"
#include "player_piece.h"

namespace wsun
{
namespace cchess
{

static const int INIT_HISTORY_STEPS_RECORD_SIZE = (2 << 10);
static const char* INIT_FEN_STRING = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";

struct step
{
	int mv;
	Piece* end_piece;
	int in_check;
	uint32_t zobrist_key;
};

struct step* step_create();
void step_release(struct step* step);

class Board
{
public:
	Board(int side = SIDE_TYPE_RED)
		: redPlayer_(new Player(SIDE_TYPE_RED)),
			blackPlayer_(new Player(SIDE_TYPE_BLACK)),
			currentSidePlayer_(side == SIDE_TYPE_RED ? redPlayer_ : blackPlayer_)
	{
		initPieceArray();
		initHistoryStepRecords();
		initFromFen(INIT_FEN_STRING);
	}

	~Board()
	{
		delete redPlayer_;
		delete blackPlayer_;
		releaseHistoryStepRecords();
	}

	void initPieceArray()
	{
		std::fill(pieces_, pieces_ + 256, nullptr);
	}

	void initHistoryStepRecords();
	void releaseHistoryStepRecords();

	Board* getExchangeSideBoard() const;
	Board* getMirrorBoard() const;

	Player* currentSidePlayer() const { return currentSidePlayer_; }
	Piece** pieces() { return pieces_; }
	const Zobrist& getZobrist() const
	{
		return zobristHelper_.getZobrist();
	}
	Zobrist getMirrorZobrist() const;
	void resetData();
	void addPieceToBoard(int type, int side, int pos);

	// 用fen串信息来初始化局面
	int initFromFen(const char* fen);
	// 根据fen串信息重置局面
	void resetFromFen(const char* fen);
	// 重置到最初局面
	void reset();

	Player* getOpponentPlayerByPlayer(Player* p) const
	{
		return (p == redPlayer_ ? blackPlayer_ : redPlayer_);
	}
	// 对手player
	Player* getOpponentPlayer() const
	{
		return getOpponentPlayerByPlayer(currentSidePlayer_);
	}
	// 更换下棋方
	void changeSide()
	{
		currentSidePlayer_ = getOpponentPlayer();
		zobristHelper_.updateByChangeSide();
	}
	// 当前局面评价函数
	int evaluate() const
	{
		return currentSidePlayer_->value() - getOpponentPlayer()->value() + 3;
	}

	bool willKillKing(Player* player);
	// 被对手将军
	bool willKillSelfKing()
	{
		return willKillKing(currentSidePlayer_);
	}
	// 将对手军
	bool willKillOpponentKing()
	{
		return willKillKing(getOpponentPlayer());
	}

	// 根据当前局面输出fen格式的局面信息
	std::string toFen();

	// 生成某棋子所有的走法(注意：不存在走完之后依然被对方将军),
	// capatured: 是否只生成吃子走法
	int generateMoves(Piece* piece, int* mvs, bool capatured);

	// 生成所有棋子所有的走法 capatured: 是否只生成吃子走法
	int generateAllMoves(int* mvs, bool capatured = false);
	int generateAllMovesNoncheck(int* mvs, bool capatured = false);

	// 困毙，是否无棋可走
	bool noWayToMove();

	void addPiece(Piece* piece, int pos);
	Piece* delPiece(int pos);

	bool legalMovePiece(Piece* piece, int dest);
	bool legalMove(int mv);

	// 真正走棋的动作
	void makeMove(int mv);

	// 撤销上一步走棋
	void undoMove();

	// 将每一步走法记录到历史表
	void makeHistoryStep(int mv, Piece* end_piece, int inCheck, uint32_t zobristKey);

	void makeNullMove()
	{
		makeHistoryStep(0, 0, 0, zobristHelper_.getZobrist().key_);
	}

	void undoNullMove()
	{
		--history_step_records_size;
	}

	// 用于AI
	int mvvLva(int mv)
	{
		int start = start_of_move(mv);
		int end = end_of_move(mv);
		
		// 必须是吃子着法
		//assert(board->pieces[start] && board->pieces[end]);

		int mvv = array_mvv_lva[pieces_[end]->type()] * 10;
		int lva = array_mvv_lva[pieces_[start]->type()];
		return mvv - lva;
	}

	// 检测重复局面
	int repetitionStatus(int recur);

	int getMachineHelp();

	// 选子提示所有走法
	int prompt(int pos, int* mvs, int capatured);

	bool isCapatured(int mv)
	{
		return pieces_[end_of_move(mv)] != NULL;
	}

	void play(int mv)
	{
		makeMove(mv);
		changeSide();
	}

	void play(const char* iccs_mv)
	{
		int mv = iccs_move_to_move(iccs_mv);
		play(mv);
	}

	void backOneStep()
	{
		undoMove();
		changeSide();
	}

private:
	Player* redPlayer_;				// 红方
	Player* blackPlayer_;			// 黑方
	Player* currentSidePlayer_;		// 当前下棋方
	Piece* pieces_[256];			// 棋盘上的所有棋子

	struct step** history_step_records;
	int history_step_records_size;
	int history_step_records_capacity;

	ZobristHelper zobristHelper_;
};

} // namespace cchess
} // namespace wsun

#endif
