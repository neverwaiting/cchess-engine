#ifndef __WSUN_CCHESS_UTILS_H__
#define __WSUN_CCHESS_UTILS_H__

#include "constants.h"
#include <stdio.h>
#include <string.h>

namespace wsun
{
namespace cchess
{

// 是否在棋盘范围内
inline static int in_board(int pos)
{
	return array_in_borad[pos] != 0;
}

// 是否在九宫格内
inline static int in_fort(int pos)
{
	return array_in_fort[pos] != 0;
}

//是否在同一边
inline static int same_half(int a, int b)
{
	return ((a ^ b) & 0x80) == 0;
}

// 是否在同一条直线上(列)
inline static int same_col(int a, int b)
{
	return ((a ^ b) & 0x0f) == 0;
}

// 是否在同一条直线上(行)
inline static int same_row(int a, int b)
{
	return ((a ^ b) & 0xf0) == 0;
}

// 根据两点是否同行或是同列计算偏移量,
// 如果不同行同列则返回零
inline static int get_offset(int a, int b)
{
	if (same_row(a, b))
	{
		return (a > b ? -1 : 1); 
	}
	else if (same_col(a, b))
	{
		return (a > b ? -16 : 16);
	}
	else
	{
		return 0;
	}
}

inline static int convert_to_pos(int row, int col)
{
	return ((row + 3) << 4) + (col + 3);
}

inline static int col_of_pos(int pos)
{
	return (pos & 0x0f) - 3;
}

inline static int row_of_pos(int pos)
{
	return (pos >> 4) - 3;
}

inline static int mirror_pos(int pos)
{
	return convert_to_pos(row_of_pos(pos), 8 - col_of_pos(pos));
}

inline static int start_of_move(int mv)
{
	return (mv & 0xff);
}

inline static int end_of_move(int mv)
{
	return (mv >> 8);
}

inline static int get_move(int start, int end)
{
	return (start | (end << 8));
}

inline static int convert_reserse_move(int mv)
{
	return get_move(end_of_move(mv), start_of_move(mv));
}

inline static int convert_mirror_move(int mv)
{
	return get_move(mirror_pos(start_of_move(mv)), mirror_pos(end_of_move(mv)));
}

// iccs move
inline static int iccs_pos_to_pos(char iccs_col, char iccs_row)
{
	int row = (int)('9' - iccs_row);
	int col = (int)(iccs_col - 'a');
	return convert_to_pos(row, col);
}

inline static int iccs_move_to_move(const char* iccs_move)
{
	int start = iccs_pos_to_pos(iccs_move[0], iccs_move[1]);
	int end = iccs_pos_to_pos(iccs_move[2], iccs_move[3]);
	return get_move(start, end);
}

// 转换为Iccs（Internet Chinese Chess Server中国象棋互联网服务器）坐标
inline static void pos_to_iccs_pos(char* iccs_pos, int pos)
{
	int col = col_of_pos(pos);
	int row = row_of_pos(pos);

	sprintf(iccs_pos, "%c%c", (char)(col + 'a'), (char)('9' - row));
}

inline static void move_to_iccs_move(char* iccs_move, int mv)
{
	pos_to_iccs_pos(iccs_move, start_of_move(mv));
	pos_to_iccs_pos(iccs_move + 2, end_of_move(mv));
}

} // namespace cchess
} // namespace wsun

#endif
