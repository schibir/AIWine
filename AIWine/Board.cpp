#include "Board.h"
#include "ChessShape.h"
#include<assert.h>
Board::Board(HashTable* _hashTable)
{
	hashTable = _hashTable;
}
//初始化棋盘
void Board::initBoard(int size)
{
	memset(board, 0, sizeof(board));

	boardSize = size;
	for (int i = 0; i < 1024; i++)
	{
		if (pointX(i) >= 4 && pointX(i) < boardSize + 4 && pointY(i) >= 4 && pointY(i) < boardSize + 4)
		{
			board[i].piece = EMPTY;
		}
		else
		{
			board[i].piece = OUTSIDE;
		}
	}
	for (int i = 0; i < 1024; i++)
	{
		if (inBoard(i))
		{
			for (int k = 0; k < 4; k++)
			{
				int ii = i - MOV[k];
				for (UCHAR p = 8; p != 0; p >>= 1)
				{
					if (board[ii].piece == OUTSIDE) board[i].pattern[k][0] |= p;
					if (board[ii].piece == OUTSIDE) board[i].pattern[k][1] |= p;
					ii -= MOV[k];
				}
				ii = i + MOV[k];
				for (UCHAR p = 16; p != 0; p <<= 1)
				{
					if (board[ii].piece == OUTSIDE) board[i].pattern[k][0] |= p;
					if (board[ii].piece == OUTSIDE) board[i].pattern[k][1] |= p;
					ii += MOV[k];
				}
			}
		}
	}
	for (int i = 0; i < 1024; i++)
	{
		if (inBoard(i))
		{
			board[i].update1(0);
			board[i].update1(1);
			board[i].update1(2);
			board[i].update1(3);
			board[i].update4();
		}
	}
	chessCount = 0;
	who = BLACK;
	opp = WHITE;
	upperLeft = makePoint(boardSize + 3, boardSize + 3);
	lowerRight = makePoint(4,4);
	memset(nShape, 0, sizeof(nShape));

	//初始化米字范围
	//8个方向
	const int range[8] = { -1,-31,-32,-33,1,31,32,33 };
	//8个八卦点
	const int horse[8] = { -65,-63,-34,-30,30,34,63,65 };
	int n3 = 0, n4 = 0;
	for (int i = 1; i <= 3; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			Range3[n3++] = i*range[j];
		}
	}
	for (int i = 0; i < 8; i++)
	{
		Range3[n3++] = horse[i];
	}
	for (int i = 1; i <= 4; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			Range4[n4++] = i*range[j];
		}
	}
}
//落子
void Board::move(Point p)
{
	assert(check());
	ply++;
	if (ply > maxPly) maxPly = ply;
	nShape[0][board[p].shape4[0]]--;
	nShape[1][board[p].shape4[1]]--;

	hashTable->move(p, who);
	
	board[p].piece = who;
	remChess[chessCount] = &board[p];
	remPoint[chessCount] = p;
	remULCand[chessCount] = upperLeft;
	remLRCand[chessCount] = lowerRight;
	chessCount++;

	int x1 = pointX(upperLeft), y1 = pointY(upperLeft);
	int x2 = pointX(lowerRight), y2 = pointY(lowerRight);

	if (pointX(p) - 2 < x1) x1 = max(pointX(p) - 2, 4);
	if (pointY(p) - 2 < y1) y1 = max(pointY(p) - 2, 4);
	if (pointX(p) + 2 > x2) x2 = min(pointX(p) + 2, boardSize + 3);
	if (pointY(p) + 2 > y2) y2 = min(pointY(p) + 2, boardSize + 3);
	upperLeft = makePoint(x1, y1);
	lowerRight = makePoint(x2, y2);

	//更新位编码以及棋型信息
	for (int k = 0; k < 4; k++)
	{
		Point move_p = p;
		//p在move_p的右边，高位更新
		for (UCHAR m = 16; m != 0; m <<= 1)
		{
			move_p -= MOV[k];
			int x = pointX(move_p) - 4;
			int y = pointY(move_p) - 4;
			//printf("点(%d,%d)\n", x, y);
			board[move_p].pattern[k][who] |= m;
			if (board[move_p].piece == EMPTY)
			{
				UCHAR &s0 = board[move_p].shape4[0];
				UCHAR &s1 = board[move_p].shape4[1];
				board[move_p].update1(k);
				nShape[0][s0]--; nShape[1][s1]--;
				board[move_p].update4();
				nShape[0][s0]++; nShape[1][s1]++;
				if (s0 == A) fivePoint[0] = move_p;
				if (s1 == A) fivePoint[1] = move_p;
			}
		}
		move_p = p;
		//p在move_p的左边，低位更新
		for (UCHAR m = 8; m != 0; m >>= 1)
		{
			move_p += MOV[k];
			board[move_p].pattern[k][who] |= m;
			if (board[move_p].piece == EMPTY)
			{
				UCHAR &s0 = board[move_p].shape4[0];
				UCHAR &s1 = board[move_p].shape4[1];
				board[move_p].update1(k);
				nShape[0][s0]--; nShape[1][s1]--;
				board[move_p].update4();
				nShape[0][s0]++; nShape[1][s1]++;
				if (s0 == A) fivePoint[0] = move_p;
				if (s1 == A) fivePoint[1] = move_p;
			}
		}
	}
	//更新8个方向，两步以内的棋子数
	for (int r:RANGE)
	{
		board[p + r].neighbor++;
	}
	
	assert(check());
	who = oppent(who);
	opp = oppent(opp);
}
//提子
void Board::undo()
{
	assert(check());
	ply--;
	chessCount--;
	Point p = remPoint[chessCount];
	upperLeft = remULCand[chessCount];
	lowerRight = remLRCand[chessCount];

	Chess* chess = remChess[chessCount];
	chess->update1(0);
	chess->update1(1);
	chess->update1(2);
	chess->update1(3);
	chess->update4();

	nShape[0][chess->shape4[0]]++;
	nShape[1][chess->shape4[1]]++;
	if (chess->shape4[0] == A) fivePoint[0] = p;
	if (chess->shape4[1] == A) fivePoint[1] = p;
	chess->piece = EMPTY;

	who = oppent(who);
	opp = oppent(opp);

	hashTable->move(p, who);

	//更新位编码以及棋型信息
	for (int k = 0; k < 4; k++)
	{
		Point move_p = p;
		//p在move_p的右边，更新move_p的高位
		for (UCHAR m = 16; m != 0 ; m <<= 1)
		{
			move_p -= MOV[k];
			board[move_p].pattern[k][who] ^= m;
			if (board[move_p].piece == EMPTY)
			{
				UCHAR &s0 = board[move_p].shape4[0];
				UCHAR &s1 = board[move_p].shape4[1];
				board[move_p].update1(k);
				nShape[0][s0]--; nShape[1][s1]--;
				board[move_p].update4();
				nShape[0][s0]++; nShape[1][s1]++;
				if (s0 == A) fivePoint[0] = move_p;
				if (s1 == A) fivePoint[1] = move_p;
			}
		}
		move_p = p;
		//p在move_p的的左边，更新move_p的低位
		for (UCHAR m = 8; m != 0; m >>= 1)
		{
			move_p += MOV[k];
			board[move_p].pattern[k][who] ^= m;
			if (board[move_p].piece == EMPTY)
			{
				UCHAR &s0 = board[move_p].shape4[0];
				UCHAR &s1 = board[move_p].shape4[1];
				board[move_p].update1(k);
				nShape[0][s0]--; nShape[1][s1]--;
				board[move_p].update4();
				nShape[0][s0]++; nShape[1][s1]++;
				if (s0 == A) fivePoint[0] = move_p;
				if (s1 == A) fivePoint[1] = move_p;
			}
		}
	}
	//更新8个方向，两步以内的棋子数
	for (int r:RANGE)
	{
		board[p + r].neighbor--;
	}
	assert(check());
}
//生成所有分支
void Board::generateCand(Cand cand[], int& nCand)
{
	nCand = 0;
	if (nShape[who][A] > 0) 
	{ 
		for (int i = upperLeft; i <= lowerRight; i++)
		{
			if (board[i].isCand() && board[i].shape4[who] == A)
			{
				nCand = 1;
				cand[0].point = i;
				return;
			}
		}
		assert(false);
	}
	
	if (nShape[opp][A] > 0)
	{
		nCand = 1;
		cand[0].point = fivePoint[opp];
		return;
	}
	if (nShape[who][B] > 0)
	{
		for (int i = upperLeft; i <= lowerRight; i++)
		{
			if (board[i].isCand() && board[i].shape4[who] == B)
			{
				nCand = 1;
				cand[0].point = i;
				return;
			}
		}
		assert(false);
	}
	//查询哈希表中的最佳点
	Point hashMove = 0;
	if (hashTable->present() && hashTable->depth() >= 0 && hashTable->best() != 0)
	{
		hashMove = hashTable->best();
		cand[0].point = hashMove;
		cand[0].value = WinScore;
		nCand = 1;
	}
	if (nShape[opp][B] > 0)
	{
		for (int i = upperLeft; i <= lowerRight; i++)
		{
			if (board[i].isCand() && i != hashMove)
			{
				if (board[i].shape4[who] >= F || board[i].shape4[opp] >= F)
				{
					cand[nCand].value = board[i].prior(who);
					cand[nCand++].point = i;
				}
			}
		}
		assert(nCand > 0 && nCand <= 256);
		return;
	}

	for (int i = upperLeft; i <= lowerRight; i++)
	{
		if (board[i].isCand() && i != hashMove)
		{
			cand[nCand].value = board[i].prior(who);
			cand[nCand].point = i;
			if (cand[nCand].value > 1) nCand++;
		}
		assert(nCand <= 256);
	}
}
//局面评估
int Board::evaluate()
{
	int p;
	int eval[2] = { 0 };
	Chess *c;
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		assert(p == BLACK || p == WHITE);
		for (int k = 0; k < 4; k++)
		{
			eval[p] += ChessShape::shapeRank[c->pattern[k][p]][c->pattern[k][1 - p]];
		}
	}
	return eval[who] - eval[opp] + 50;
}
int Board::evaluateTest2()
{
	int p;
	int eval[2] = { 0 };
	Chess *c;
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		assert(p == BLACK || p == WHITE);
		c->updatePiece4();
		eval[p] += ChessShape::fourShapeScore[c->shape4[p]];
	}
	return eval[who] - eval[opp];
}
int Board::evaluateTest3()
{
	int p, pos;
	int eval[2] = { 0 };
	bool record[1024][4] = { 0 };
	int shape[2][10] = { 0 };
	int fourShape[2][16] = { 0 };
	Chess *c;
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		pos = remPoint[i];
		
		for (int k = 0; k < 4; k++)
		{
			c->updatePiece1(k);
			if (record[pos][k]) continue;//已统计过
			shape[p][c->shape[k][p]]++;
			//把相关的棋子设为true
			int newPos = pos;
			for (int j = 0; j < 4; j++)
			{
				newPos += MOV[k];
				if (board[newPos].piece == p) record[newPos][k] = true;
				else if (board[newPos].piece != EMPTY) break;
			}
			newPos = pos;
			for (int j = 0; j < 4; j++)
			{
				newPos -= MOV[k];
				if (board[newPos].piece == p) record[newPos][k] = true;
				else if (board[newPos].piece != EMPTY) break;
			}
		}
		assert(p == BLACK || p == WHITE);
	}
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		c->updatePiece4();
		int temp[10] = { 0 };
		temp[c->shape[0][p]]++;
		temp[c->shape[1][p]]++;
		temp[c->shape[2][p]]++;
		temp[c->shape[3][p]]++;
		switch (c->shape4[p])
		{
		case K:
			shape[p][BLOCK3] -= temp[BLOCK3];
			shape[p][FLEX2] -= temp[FLEX2];
			fourShape[p][K]++;
			break;
		case I:
			shape[p][FLEX3] --;
			shape[p][BLOCK2] -= temp[BLOCK2];
			fourShape[p][I]++;
			break;
		case H:
			shape[p][FLEX3] --;
			shape[p][FLEX2] -= temp[FLEX2];
			shape[p][BLOCK3] -= temp[BLOCK3];
			fourShape[p][H]++;
			break;
		case G:
			shape[p][FLEX3] -= temp[FLEX3];
			fourShape[p][G]++;
			break;
		case E:
			shape[p][BLOCK4] --;
			shape[p][BLOCK2] -= temp[BLOCK2];
			fourShape[p][E]++;
			break;
		case D:
			shape[p][BLOCK4] --;
			shape[p][BLOCK3] -= temp[BLOCK3];
			shape[p][FLEX2] -= temp[FLEX2];
			fourShape[p][D]++;
			break;
		case C:
			shape[p][BLOCK4] --;
			shape[p][FLEX3] -= temp[FLEX3];
			fourShape[p][C]++;
			break;
		case B:
			fourShape[p][B]++;
			if (temp[BLOCK4] >= 2)
			{
				shape[p][BLOCK4] -= temp[BLOCK4];
			}
		default:
			break;
		}
	}
	int convert[10] = { 0,0,0,N,M,L,J,F,B,A };
	for (int i = 1; i < 10; i++)
	{
		int index = convert[i];
		fourShape[0][index] += shape[0][i];
		fourShape[1][index] += shape[1][i];
	}
	for (int i = 1; i < 14; i++)
	{
		eval[0] += fourShape[0][i] * ChessShape::fourShapeScore[i];
		eval[1] += fourShape[1][i] * ChessShape::fourShapeScore[i];
	}
	return eval[who] - eval[opp];
}
int Board::evaluateDebug3()
{
	int p, pos;
	int eval[2] = { 0 };
	bool record[1024][4] = { 0 };
	int shape[2][10] = { 0 };
	int fourShape[2][16] = { 0 };
	Chess *c;
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		pos = remPoint[i];

		for (int k = 0; k < 4; k++)
		{
			c->updatePiece1(k);
			if (record[pos][k]) continue;//已统计过
			shape[p][c->shape[k][p]]++;
			//把相关的棋子设为true
			int newPos = pos;
			for (int j = 0; j < 4; j++)
			{
				newPos += MOV[k];
				if (board[newPos].piece == p) record[newPos][k] = true;
				else if (board[newPos].piece != EMPTY) break;
			}
			newPos = pos;
			for (int j = 0; j < 4; j++)
			{
				newPos -= MOV[k];
				if (board[newPos].piece == p) record[newPos][k] = true;
				else if (board[newPos].piece != EMPTY) break;
			}
		}
		assert(p == BLACK || p == WHITE);
	}
	for (int i = 0; i < chessCount; i++)
	{
		c = remChess[i];
		p = c->piece;
		c->updatePiece4();
		int temp[10] = { 0 };
		temp[c->shape[0][p]]++;
		temp[c->shape[1][p]]++;
		temp[c->shape[2][p]]++;
		temp[c->shape[3][p]]++;
		switch (c->shape4[p])
		{
		case K:
			shape[p][BLOCK3]-=temp[BLOCK3];
			shape[p][FLEX2]-=temp[FLEX2];
			fourShape[p][K]++;
			break;
		case I:
			shape[p][FLEX3] --;
			shape[p][BLOCK2] -= temp[BLOCK2];
			fourShape[p][I]++;
			break;
		case H:
			shape[p][FLEX3] --;
			shape[p][FLEX2] -= temp[FLEX2];
			shape[p][BLOCK3] -= temp[BLOCK3];
			fourShape[p][H]++;
			break;
		case G:
			shape[p][FLEX3] -= temp[FLEX3];
			fourShape[p][G]++;
			break;
		case E:
			shape[p][BLOCK4] --;
			shape[p][BLOCK2] -= temp[BLOCK2];
			fourShape[p][E]++;
			break;
		case D:
			shape[p][BLOCK4] --;
			shape[p][BLOCK3] -= temp[BLOCK3];
			shape[p][FLEX2] -= temp[FLEX2];
			fourShape[p][D]++;
			break;
		case C:
			shape[p][BLOCK4] --;
			shape[p][FLEX3] -= temp[FLEX3];
			fourShape[p][C]++;
			break;
		case B:
			fourShape[p][B]++;
			if (temp[BLOCK4]>=2)
			{
				shape[p][BLOCK4] -= temp[BLOCK4];
			}
		default:
			break;
		}
	}
	int convert[10] = { 0,0,0,N,M,L,J,F,B,A };
	for (int i = 1; i < 10; i++)
	{
		int index = convert[i];
		fourShape[0][index] += shape[0][i];
		fourShape[1][index] += shape[1][i];
	}
	string name[16] = { "none","眠二","活二","眠三","活二加眠三","活三","活三加眠二","活三加眠三或活二","双活三","冲四","冲四加眠二","冲四加眠三或活二","冲四加活三","活四","连五" ,"禁手" };
	for (int shape4 = 1; shape4 <14; shape4++)
	{
		int whoEval = fourShape[who][shape4] * ChessShape::fourShapeScore[shape4];
		int oppEval = fourShape[opp][shape4] * ChessShape::fourShapeScore[shape4];
		if (whoEval != 0) cout << "MESSAGE 本方 棋型：" << name[shape4] << " 个数" << fourShape[who][shape4] << " 分值：" << whoEval << endl;
		if (oppEval != 0) cout << "MESSAGE 对方 棋型：" << name[shape4] << " 个数" << fourShape[opp][shape4] << " 分值：" << oppEval << endl;
		eval[who] += whoEval;
		eval[opp] += oppEval;
	}
	return eval[who] - eval[opp];
}
int Board::evaluateTest()
{
	int eval[2] = { 0 };
	for (int shape4 = 1; shape4 <15; shape4++)
	{
		eval[0] += nShape[0][shape4]*ChessShape::fourShapeScore[shape4];
		eval[1] += nShape[1][shape4]*ChessShape::fourShapeScore[shape4];
	}
	return eval[who] - eval[opp] + 50;
}
int Board::evaluateDebug()
{
	int eval[2] = { 0 };
	string name[16] = { "none","none","none","none","活二加眠三","活三","活三加眠二","活三加眠三或活二","双活三","冲四","冲四加眠二","冲四加眠三或活二","冲四加活三","活四","连五" ,"禁手" };
	for (int shape4 = 1; shape4 <15; shape4++)
	{
		int whoEval = nShape[who][shape4] * ChessShape::fourShapeScore[shape4];
		int oppEval = nShape[opp][shape4] * ChessShape::fourShapeScore[shape4];
		if (whoEval > 0) cout << "MESSAGE 本方 棋型：" << name[shape4] << " 个数" << nShape[who][shape4] << " 分值：" << whoEval << endl;
		if (oppEval > 0) cout << "MESSAGE 对方 棋型：" << name[shape4] << " 个数" << nShape[opp][shape4] << " 分值：" << oppEval << endl;
		eval[who] += whoEval;
		eval[opp] += oppEval;
	}
	return eval[who] - eval[opp];
}

//胜利局面搜索(将在几步内赢棋)
int Board::quickWinSearch()
{
	int q;
	if (nShape[who][A] >= 1) return 1;   
	if (nShape[opp][A] >= 2) return -2;  
	if (nShape[opp][A] == 1)             
	{
		move(fivePoint[opp]);
		q = -quickWinSearch();
		undo();
		if (q < 0) q--; else if (q > 0) q++;
		return q;
			
	}
	if (nShape[who][B] >= 1) return 3;   
	if (nShape[who][C] >= 1)             // XOOO_ * _OO
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0) return 5;
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand()&&board[m].shape4[who] == C)
			{
				
				move(m);
				q = -quickWinSearch();
				undo();
				if (q > 0)
				{
					return q + 1;
				}
				
			}
		}
	}
	if (nShape[who][G] >= 1)
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			return 5;
		}
	}
	return 0;
}
int Board::vcfSearch(int *winPoint)
{
	Point lastPoint = findLastPoint();
	if (lastPoint == -1) return 0;
	long vcf_start = getTime();
	vcfNode = 0;
	int result = vcfSearch(who, 0, lastPoint, winPoint);
	long vcfTime = getTime() - vcf_start;
	cout << "MESSAGE VCF花费时间：" << vcfTime << "ms 节点数：" << vcfNode << endl;
	return result;
}
//vcf搜索
int Board::vcfSearch(int searcher,int depth,int lastPoint)
{
	int q;
	if (nShape[who][A] >= 1) return 1;
	if (nShape[opp][A] >= 2) return -2;
	//对方下一步能成五，挡在成五点
	if (nShape[opp][A] == 1)
	{
		move(fivePoint[opp]);
		q = -vcfSearch(searcher, depth + 1, lastPoint);
		undo();
		if (q < 0) q--; else if (q > 0) q++;
		return q;
	}
	//本方能成活四,三步胜利
	if (nShape[who][B] >= 1) return 3;
	//本方有冲四活三，尝试
	if (who==searcher&&nShape[who][C] >= 1)             
	{
		//对方没有能成四的点，五步胜利
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0) return 5;
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == C)
			{
				move(m);
				q = -vcfSearch(searcher, depth + 1, m);
				undo();
				if (q > 0) return q + 1;

			}
		}
	}
	//本方有冲四和其他棋型(活二、眠三、眠二中的一個)
	if (who==searcher&&depth<MAX_VCF_DEPTH && nShape[who][D] + nShape[who][E] >= 1)
	{
		for (int r:Range4)
		{
			int m = lastPoint + r;
			if (board[m].isCand() && (board[m].shape4[who] == D || board[m].shape4[who]==E))
			{
				move(m);
				q = -vcfSearch(searcher, depth + 1, m);
				undo();
				if (q > 0) return q + 1;
			}
		}
	}
	//本方能成双活三，对方没有任何冲四或活四,五步胜利
	if (who==searcher&&nShape[who][G] >= 1)
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0) return 5;
	}
	return 0;
}

//vcf搜索
int Board::vcfSearch(int searcher, int depth,int lastPoint,int *winPoint)
{
	int q;
	vcfNode++;
	if (nShape[who][A] >= 1)
	{
		if (depth == 0) *winPoint = findPoint(who, A);
		return 1;
	}
	if (nShape[opp][A] >= 2) return -2;
	//对方下一步能成五，挡在成五点
	if (nShape[opp][A] == 1)
	{
		move(fivePoint[opp]);
		q = -vcfSearch(searcher, depth + 1, lastPoint, winPoint);
		undo();
		if (q < 0) q--;
		else if (q > 0)
		{
			if (depth == 0) *winPoint = fivePoint[opp];
			q++;
		}
		return q;
	}
	//本方能成活四,三步胜利
	if (nShape[who][B] >= 1)
	{
		if (depth == 0) *winPoint = findPoint(who, B);
		return 3;
	}
	//本方有冲四活三，尝试
	if (who == searcher&&nShape[who][C] >= 1)
	{
		//对方没有能成四的点，五步胜利
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			if (depth == 0) *winPoint = findPoint(who, C);
			return 5;
		}
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == C)
			{
				move(m);
				q = -vcfSearch(searcher, depth + 1,m, winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;
				}
			}
		}
	}
	//本方有冲四和其他棋型(活二、眠三、眠二中的一個)
	if (who == searcher&&depth<MAX_VCF_DEPTH && nShape[who][D] + nShape[who][E] >= 1)
	{
		for (int r : Range4)
		{
			int m = lastPoint + r;
			if (board[m].isCand() && (board[m].shape4[who] == D || board[m].shape4[who] == E))
			{
				move(m);
				q = -vcfSearch(searcher, depth + 1,m, winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;
				}
			}
		}
	}
	//本方能成双活三，对方没有任何冲四或活四,五步胜利
	if (who == searcher&&nShape[who][G] >= 1)
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			if (depth == 0) *winPoint = findPoint(who, G);
			return 5;
		}
	}
	return 0;
}
Point Board::findPoint(Piece piece, FourShape shape)
{
	for (int m = upperLeft; m < lowerRight; m++)
	{
		if (board[m].isCand() && board[m].shape4[piece] == shape)
		{
			return m;
		}
	}
	return -1;
}
Point Board::findLastPoint()
{
	if (chessCount < 2) return -1;
	for (int i = chessCount - 2; i >= 0; i -= 2)
	{
		if (remChess[i]->shape[0][who] >= FLEX2 || remChess[i]->shape[1][who] >= FLEX2 || remChess[i]->shape[2][who] >= FLEX2 || remChess[i]->shape[3][who] >= FLEX2)
		{
			return remPoint[i];
		}
	}
	return -1;
}
int Board::vctSearch(int *winPoint)
{
	t_VCT_Start = getTime();
	vctNode = 0;
	vctStop = false;
	int result = 0;
	int depth;
	Point lastPoint = findLastPoint();
	if (lastPoint == -1) return 0;
	for (depth = 10; depth <= MAX_VCT_DEPTH; depth+=2)
	{
		result = vctSearch(who, 0, depth, lastPoint, winPoint);
		if (result > 0||vctStop)
		{
			break;
		}
	}
	long vctTime = getTime() - t_VCT_Start;
	cout << "MESSAGE VCT花费时间：" << vctTime << "ms 节点数：" << vctNode << " 层数：" << __min(depth,MAX_VCT_DEPTH) << " 超时停止：" << vctStop << endl;
	return result;
}
//VCT搜索
int Board::vctSearch(int searcher, int depth, int maxDepth, int lastPoint)
{
	static int cnt;
	if (--cnt<0)
	{
		cnt = 1000;
		if (getTime() - t_VCT_Start > MAX_VCT_TIME)
		{
			vctStop = true;
		}
	}
	int q;
	//本方能成五
	if (nShape[who][A] >= 1) return 1;
	if (nShape[opp][A] >= 2) return -2;
	//对方下一步能成五，挡在成五点
	if (nShape[opp][A] == 1)
	{
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[opp] == A)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, lastPoint);
				undo();
				if (q < 0) q--;
				else if (q > 0) q++;
				return q;
			}
		}

	}
	//本方能成活四,三步胜利
	if (nShape[who][B] >= 1) return 3;
	//大于最大深度不再扩展
	if (depth > maxDepth || vctStop) return 0;
	//对方是算杀方且能活四，防守
	if (who != searcher&&nShape[opp][B] >= 1)
	{
		int max_q = -1000;
		for (int m = upperLeft; m <= lowerRight; m++)
		{
			if (board[m].isCand() && (board[m].shape4[opp] >= F || board[m].shape4[who] >= F))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, lastPoint);
				undo();
				if (q > 0) return q + 1;//有个防守点能赢，就算赢
				else if (q == 0) return 0;
				else if (q > max_q) max_q = q;
			}
		}
		return max_q;//q>0时，会提前返回，剩下的情况，如果等于0，返回0，否则返回最大的q值，所以直接返回max_q即可
	}
	//本方是算杀方，有冲四活三，尝试
	if (who == searcher&&nShape[who][C] >= 1)
	{
		//对方没有能成四的点，五步胜利
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			return 5;
		}
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == C)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m);
				undo();
				if (q > 0) return q + 1;

			}
		}
	}
	//攻击方尝试双活三
	if (who == searcher&&nShape[who][G]>0)
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			return 5;
		}
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == G)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m);
				undo();
				if (q > 0) return q + 1;
			}
		}
	}
	//本方是算杀方,尝试剩下的所有冲四点（除掉冲四活三)
	if (who == searcher&&nShape[who][D] + nShape[who][E] >= 1)
	{
		//后续算杀只考虑米字范围，防止vct爆炸
		for (int r : Range4)
		{
			int m = lastPoint + r;
			if (board[m].isCand() && (board[m].shape4[who] == D|| board[m].shape4[who] == E))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m);
				undo();
				if (q > 0) return q + 1;
			}
		}
	}

	//尝试活三加其他棋型
	if (who == searcher && nShape[who][H] + nShape[who][I] >= 1)
	{
		//后续算杀只考虑米字范围，防止vct爆炸
		for (int r : Range3)
		{
			int m = lastPoint + r;
			if (board[m].isCand() && (board[m].shape4[who] == H|| board[m].shape4[who] == I))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m);
				undo();
				if (q > 0) return q + 1;
			}
		}
	}

	return 0;
}
//VCT搜索
int Board::vctSearch(int searcher,int depth,int maxDepth,int lastPoint,int* winPoint)
{
	static int cnt;
	if (--cnt<0)
	{
		cnt = 1000;
		if (getTime() - t_VCT_Start > MAX_VCT_TIME)
		{
			vctStop = true;
		}
	}
	vctNode++;
	int q;
	//本方能成五
	if (nShape[who][A] >= 1)
	{
		if (depth == 0) *winPoint = findPoint(who, A);
		return 1;
	}
	if (nShape[opp][A] >= 2) return -2;
	//对方下一步能成五，挡在成五点
	if (nShape[opp][A] == 1)
	{
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[opp] == A)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, lastPoint, winPoint);
				undo();
				if (q < 0) q--; 
				else if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					q++;
				}
				return q;
			}
		}

	}
	//本方能成活四,三步胜利
	if (nShape[who][B] >= 1)
	{
		if (depth == 0) *winPoint = findPoint(who, B);
		return 3;
	}
	//大于最大深度，不再扩展
	if (depth > maxDepth || vctStop) return 0;
	//对方是算杀方且能活四，防守
	if (who != searcher&&nShape[opp][B] >= 1)
	{
		int max_q = -1000;
		for (int m = upperLeft; m <= lowerRight; m++)
		{
			if (board[m].isCand() && (board[m].shape4[opp] >= F||board[m].shape4[who]>=F))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, lastPoint, winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;//有个防守点能赢，就算赢
				}
				else if (q == 0) return 0;
				else if (q > max_q) max_q = q;
			}
		}
		return max_q;//q>0时，会提前返回，剩下的情况，如果等于0，返回0，否则返回最大的q值，所以直接返回max_q即可
	}
	//本方是算杀方，有冲四活三，尝试
	if (who==searcher&&nShape[who][C] >= 1)
	{
		//对方没有能成四的点，五步胜利
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			if (depth == 0) *winPoint = findPoint(who, C);
			return 5;
		}
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == C)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m,winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;
				}

			}
		}
	}	
	//攻击方尝试双活三
	if (who == searcher&&nShape[who][G]>0)
	{
		if (nShape[opp][B] == 0 && nShape[opp][C] == 0 && nShape[opp][D] == 0 && nShape[opp][E] == 0 && nShape[opp][F] == 0)
		{
			if (depth == 0) *winPoint = findPoint(who, G);
			return 5;
		}
		for (int m = upperLeft; m < lowerRight; m++)
		{
			if (board[m].isCand() && board[m].shape4[who] == G)
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m, winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;
				}
			}
		}
	}
	//本方是算杀方,尝试剩下的所有冲四点（除掉冲四活三)
	if(who==searcher&&nShape[who][D] + nShape[who][E] >= 1)
	{
		//后续算杀只考虑米字范围，防止vct爆炸
		for (int r:Range4)
		{
			int m = lastPoint + r;
			if (board[m].isCand() && (board[m].shape4[who] == D|| board[m].shape4[who] == E))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m, winPoint);
				undo();
				if (q > 0)
				{
					if (depth == 0) *winPoint = m;
					return q + 1;
				}
					
			}
		}
	}
	
	//尝试活三加其他棋型
	if (who== searcher && nShape[who][H] + nShape[who][I] >= 1)
	{
		//后续算杀只考虑米字范围，防止vct爆炸
		for (int r:Range3)
		{
			int m = lastPoint +r;
			if (board[m].isCand() && (board[m].shape4[who] == H|| board[m].shape4[who] == I))
			{
				move(m);
				q = -vctSearch(searcher, depth + 1, maxDepth, m, winPoint);
				undo();
				if (q > 0)
				{
					*winPoint = m;
					return q + 1;
				}
			}
		}
	}

	return 0;
}
//获取所有空的点
void Board::getEmptyCand(Cand cand[], int &nCand)
{
	for (int m = upperLeft; m < lowerRight; m++)
	{
		if (board[m].piece==EMPTY)
		{
			cand[nCand++] = { m,0 };
		}
	}
}
//检查棋型数量是否正确
bool Board::check()
{
	int n[2][10] = { 0 };
	for (int m = 0; m < 1024; m++)
	{
		if (board[m].piece==EMPTY)
		{
			n[0][board[m].shape4[0]]++;
			n[1][board[m].shape4[1]]++;
		}
	}
	for (int i = 0; i < 2; i++)
		for (int j = 1; j < 10; j++)
			if (n[i][j] != nShape[i][j])
			{
				return false;
			}
	return true;
}