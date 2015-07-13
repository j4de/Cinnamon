/*
    Cinnamon is a UCI chess engine
    Copyright (C) Giuseppe Cannella

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atomic>
#include "ChessBoard.h"

using namespace _eval;

class GenMoves : public virtual ChessBoard {

public:


    GenMoves();


    virtual ~GenMoves();


    void setPerft(const bool b);

    bool generateCaptures(const int side, u64, u64);

    bool generateCapturesMoves();

    void generateMoves(const int side, const u64);

    template<int side>
    void generateMoves(const u64 allpieces) {
        ASSERT_RANGE(side, 0, 1);
        ASSERT(chessboard[KING_BLACK]);
        ASSERT(chessboard[KING_WHITE]);
        tryAllCastle(side, allpieces);
        performDiagShift(BISHOP_BLACK + side, side, allpieces);
        performRankFileShift(ROOK_BLACK + side, side, allpieces);
        performRankFileShift(QUEEN_BLACK + side, side, allpieces);
        performDiagShift(QUEEN_BLACK + side, side, allpieces);
        performPawnShift<side>(~allpieces);
        performKnightShiftCapture(KNIGHT_BLACK + side, ~allpieces, side);
        performKingShiftCapture(side, ~allpieces);
    }

    template<int side>
    bool generateCaptures(const u64 enemies, const u64 friends) {
        ASSERT_RANGE(side, 0, 1);
        ASSERT(chessboard[KING_BLACK]);
        ASSERT(chessboard[KING_WHITE]);
        u64 allpieces = enemies | friends;
        if (performPawnCapture<side>(enemies)) {
            return true;
        }
        if (performKingShiftCapture(side, enemies)) {
            return true;
        }
        if (performKnightShiftCapture(KNIGHT_BLACK + side, enemies, side)) {
            return true;
        }
        if (performDiagCapture(BISHOP_BLACK + side, enemies, side, allpieces)) {
            return true;
        }
        if (performRankFileCapture(ROOK_BLACK + side, enemies, side, allpieces)) {
            return true;
        }
        if (performRankFileCapture(QUEEN_BLACK + side, enemies, side, allpieces)) {
            return true;
        }
        if (performDiagCapture(QUEEN_BLACK + side, enemies, side, allpieces)) {
            return true;
        }
        return false;
    }

    bool getForceCheck() {
        return forceCheck;
    }

    void setForceCheck(bool b) {
        forceCheck = b;
    }

    int getMoveFromSan(const string fenStr, _Tmove *move);

    void init();

    virtual int loadFen(string fen = "");

    u64 performDiagCaptureCount(const int, const u64 allpieces);

    void takeback(_Tmove *move, const u64 oldkey, bool rep);

    void setRepetitionMapCount(int i);

    int performDiagShiftCount(const int, const u64 allpieces);

    bool performKingShiftCapture(int side, const u64 enemies);

    bool performKnightShiftCapture(const int piece, const u64 enemies, const int side);

    bool performDiagCapture(const int piece, const u64 enemies, const int side, const u64 allpieces);

    u64 getTotMoves();

    bool performRankFileCapture(const int piece, const u64 enemies, const int side, const u64 allpieces);

    template<int side>
    bool performPawnCapture(const u64 enemies);

    template<int side>
    void performPawnShift(const u64 xallpieces);

    void clearKillerHeuristic();

    int performPawnShiftCount(int side, const u64 xallpieces);

    void performDiagShift(const int piece, const int side, const u64 allpieces);

    void performRankFileShift(const int piece, const int side, const u64 allpieces);

    bool makemove(_Tmove *move, bool rep = true, bool = false);

    //bool isPinned(const int side, const uchar Position, const uchar piece);
    void incListId() {
        listId++;
#ifdef DEBUG_MODE
        if (listId < 0 || listId >= MAX_PLY) {
            display();
        }
        ASSERT_RANGE(listId, 0, MAX_PLY - 1);
#endif
    }

    void decListId() {
        gen_list[listId--].size = 0;
    }

    int getListSize() {
        return gen_list[listId].size;
    }

    void pushStackMove() {
        pushStackMove(chessboard[ZOBRISTKEY_IDX]);
    }

    void resetList() {
        gen_list[listId].size = 0;
    }

    void incKillerHeuristic(const int from, const int to, const int value) {
        if (!getRunning()) {
            return;
        }
        ASSERT_RANGE(from, 0, 63);
        ASSERT_RANGE(to, 0, 63);
        ASSERT(killerHeuristic[from][to] <= killerHeuristic[from][to] + value);
        killerHeuristic[from][to] += value;
    }

    void setRunningThread(int t) {
        runningThread = t;
    }

    _Tmove *getNextMove();

#ifdef DEBUG_MODE
    unsigned nCutAB, nNullMoveCut, nCutFp, nCutRazor;
    double betaEfficiency;
#endif
protected:
    int listId;
    _TmoveP *gen_list;
    static const u64 RANK_1 = 0xff00ULL;
    static const u64 RANK_3 = 0xff000000ULL;
    static const u64 RANK_4 = 0xff00000000ULL;
    static const u64 RANK_6 = 0xff000000000000ULL;
    static const uchar STANDARD_MOVE_MASK = 0x3;
    static const uchar ENPASSANT_MOVE_MASK = 0x1;
    static const uchar PROMOTION_MOVE_MASK = 0x2;
    static const int MAX_REP_COUNT = 1024;
    static const int NO_PROMOTION = -1;
    int repetitionMapCount;

    u64 *repetitionMap;
    int currentPly;
    bool perftMode;
    u64 numMoves = 0;
    u64 numMovesq = 0;

    _Tmove *getNextMove(decltype(gen_list));

    u64 getKingAttackers(const int xside, u64, int);

    int getMobilityRook(const int position, const u64 enemies, const u64 friends);

    int getMobilityPawns(const int side, const int ep, const u64 ped_friends, const u64 enemies, const u64 xallpieces);

    int getMobilityCastle(const int side, const u64 allpieces);

    int getMobilityQueen(const int position, const u64 enemies, const u64 friends);

    template<int side>
    bool attackSquare(const uchar Position, u64);

    void initKillerHeuristic();


    void pushRepetition(u64);

    int killerHeuristic[64][64];

    template<int side>
    bool inCheck(const int from, const int to, const uchar type, const int pieceFrom, const int pieceTo, int promotionPiece);

    void performCastle(const int side, const uchar type);

    void unPerformCastle(const int side, const uchar type);

    void tryAllCastle(const int side, const u64 allpieces);


    template<uchar type>
    bool pushmove(const int from, const int to, const int side, int promotionPiece, int pieceFrom);

    _Tmove *getMove(int i) {
        return &gen_list[listId].moveList[i];
    }

    void setRunning(int t) {
        running = t;
    }

    int getRunning() {
        return !runningThread ? 0 : running;
    }

    template<int side>
    bool inCheck() {
        return attackSquare<side>(BITScanForward(chessboard[KING_BLACK + side]));
    }

    template<int side>
    bool attackSquare(const uchar position) {
        return attackSquare<side>(position, getBitBoard<BLACK>() | getBitBoard<WHITE>());
    }

    void setKillerHeuristic(const int from, const int to, const int value) {
        if (!getRunning()) {
            return;
        }
        ASSERT_RANGE(from, 0, 63);
        ASSERT_RANGE(to, 0, 63);
        killerHeuristic[from][to] = value;
    }


private:
    int running;
    int runningThread;
    bool forceCheck = false;
    static const int MAX_MOVE = 130;
    static const u64 TABJUMPPAWN = 0xFF00000000FF00ULL;
    static const u64 TABCAPTUREPAWN_RIGHT = 0xFEFEFEFEFEFEFEFEULL;
    static const u64 TABCAPTUREPAWN_LEFT = 0x7F7F7F7F7F7F7F7FULL;

    template<int side>
    void checkJumpPawn(u64 x, const u64 xallpieces);

    int performRankFileCaptureCount(const int, const u64 enemies, const u64 allpieces);

    int performRankFileShiftCount(const int piece, const u64 allpieces);

    void popStackMove() {
        ASSERT(repetitionMapCount > 0);
        if (--repetitionMapCount && repetitionMap[repetitionMapCount - 1] == 0) {
            repetitionMapCount--;
        }
    }

    void pushStackMove(u64 key) {
        ASSERT(repetitionMapCount < MAX_REP_COUNT - 1);
        repetitionMap[repetitionMapCount++] = key;
    }
};

