/*
    Cinnamon UCI chess engine
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
#include <unistd.h>
#include "IterativeDeeping.h"

#ifdef DEBUG_MODE

#include "Hash.h"
#include "SearchManager.h"

#endif

IterativeDeeping::IterativeDeeping() : maxDepth(MAX_PLY), openBook(nullptr), ponderEnabled(false) {
    setUseBook(false);
}

void IterativeDeeping::setMaxDepth(int d) {
    maxDepth = d;
}

bool IterativeDeeping::getGtbAvailable() {
    return searchManager.getGtbAvailable();
}

IterativeDeeping::~IterativeDeeping() {
    if (openBook) {
        delete openBook;
    }
}

void IterativeDeeping::enablePonder(bool b) {
    ponderEnabled = b;
}

bool IterativeDeeping::getPonderEnabled() {
    return ponderEnabled;
}

bool IterativeDeeping::getUseBook() {
    return useBook;
}

void IterativeDeeping::loadBook(const string &f) {
    if (!openBook) {
        openBook = &OpenBook::getInstance();
    }
    useBook = openBook->load(f);
    if (!useBook) {
        openBook = nullptr;
    }
}

void IterativeDeeping::setUseBook(bool b) {
    useBook = b;
    bool valid = true;
    if (b && openBook == nullptr) {
        openBook = &OpenBook::getInstance();
        valid = useBook = openBook->load();
    }
    if ((!b && openBook) || !valid) {
        openBook = nullptr;
        useBook = false;
    }
}

void IterativeDeeping::run() {
    lock_guard<mutex> lock(commandMutex);
    int timeTaken = 0;
    searchManager.setRunning(2);
    searchManager.setRunningThread(true);
    int mply = 0;
    if (useBook) {
        ASSERT(openBook);
        string obMove = openBook->search(searchManager.boardToFen());
        if (!obMove.empty()) {
            _Tmove move;
            searchManager.getMoveFromSan(obMove, &move);
            searchManager.makemove(&move);
            cout << "bestmove " << obMove << endl;
            return;
        }
    }
    int sc = 0;
    u64 totMoves;

    mply = 0;

    searchManager.startClock();
    searchManager.clearKillerHeuristic();
    searchManager.clearAge();
    searchManager.setForceCheck(false);

    auto start1 = std::chrono::high_resolution_clock::now();
    bool inMate = false;
    int extension = 0;
    string bestmove;
    string ponderMove;
    searchManager.init();
    int mateIn = INT_MAX;
    string pvv;
    _Tmove resultMove;
    while (searchManager.getRunning(0) /*&& mateIn == INT_MAX && mply < maxDepth*/) {
//        mateIn = INT_MAX;
        totMoves = 0;
        ++mply;
        searchManager.init();

        searchManager.search(mply);

        searchManager.setRunningThread(1);
        searchManager.setRunning(1);
//        if (mply == 2) {
//            searchManager.setRunningAll(1);
//        }

        if (!searchManager.getRes(resultMove, ponderMove, pvv, &mateIn)) {
            debug<LOG_LEVEL::DEBUG, false>(LINE_INFO, "IterativeDeeping cmove == 0, exit");
            break;
        }

        searchManager.incKillerHeuristic(resultMove.from, resultMove.to, 0x800);

        auto end1 = std::chrono::high_resolution_clock::now();
        timeTaken = Time::diffTime(end1, start1);
        totMoves += searchManager.getTotMoves();

        sc = resultMove.score;
        if (resultMove.score > _INFINITE - MAX_PLY) {
            sc = 0x7fffffff;
        }
#ifdef DEBUG_MODE
        int totStoreHash = hash.nRecordHashA + hash.nRecordHashB + hash.nRecordHashE + 1;
        int percStoreHashA = hash.nRecordHashA * 100 / totStoreHash;
        int percStoreHashB = hash.nRecordHashB * 100 / totStoreHash;
        int percStoreHashE = hash.nRecordHashE * 100 / totStoreHash;
        int totCutHash = hash.n_cut_hashA + hash.n_cut_hashB + 1;
        int percCutHashA = hash.n_cut_hashA * 100 / totCutHash;
        int percCutHashB = hash.n_cut_hashB * 100 / totCutHash;
        cout << "\ninfo string ply: " << mply << "\n";
        cout << "info string tot moves: " << totMoves << "\n";
        unsigned cumulativeMovesCount = searchManager.getCumulativeMovesCount();
        cout << "info string hash stored " << totStoreHash * 100 / (1 + cumulativeMovesCount) << "% (alpha=" << percStoreHashA << "% beta=" << percStoreHashB << "% exact=" << percStoreHashE << "%)" << endl;
        // ASSERT(totStoreHash <= cumulativeMovesCount);
        cout << "info string cut hash " << totCutHash * 100 / (1 + searchManager.getCumulativeMovesCount()) << "% (alpha=" << percCutHashA << "% beta=" << percCutHashB << "%)" << endl;
        //ASSERT(totCutHash <= cumulativeMovesCount);
        u64 nps = 0;
        if (timeTaken) {
            nps = totMoves * 1000 / timeTaken;
        }
        int nCutAB = searchManager.getNCutAB();
        double betaEfficiency = searchManager.getBetaEfficiency();
        int LazyEvalCuts = searchManager.getLazyEvalCuts();
        int nCutFp = searchManager.getNCutFp();
        int nCutRazor = searchManager.getNCutRazor();
        int nHashCutFailed = searchManager.getNCutRazor();
        int nNullMoveCut = hash.cutFailed;
        unsigned totGen = searchManager.getTotGen();
        if (nCutAB) {
            cout << "info string beta efficiency: " << (int) (betaEfficiency / totGen * 10) << "%\n";
            betaEfficiency = totGen = 0.0;
        }
        cout << "info string millsec: " << timeTaken << "  (" << nps / 1000 << "k nodes per seconds) \n";
        cout << "info string alphaBeta cut: " << nCutAB << "\n";
        cout << "info string lazy eval cut: " << LazyEvalCuts << "\n";
        cout << "info string futility pruning cut: " << nCutFp << "\n";
        cout << "info string razor cut: " << nCutRazor << "\n";
        cout << "info string null move cut: " << nNullMoveCut << "\n";
        cout << "info string hash cut failed : " << nHashCutFailed << "\n";
#endif
        ///is valid move?
        bool print = true;
        if (abs(sc) > _INFINITE - MAX_PLY) {
            bool b = searchManager.getForceCheck();
            u64 oldKey = searchManager.getZobristKey(0);
            searchManager.setForceCheck(true);
            bool valid = searchManager.makemove(&resultMove);
            if (!valid) {
                extension++;
                print = false;
            }
            searchManager.takeback(&resultMove, oldKey, true);
            searchManager.setForceCheck(b);
        }
        if (print) {

            resultMove.capturedPiece = searchManager.getPieceAt(resultMove.side ^ 1, POW2[resultMove.to]);
            bestmove = Search::decodeBoardinv(resultMove.type, resultMove.from, resultMove.side);
            if (!(resultMove.type & (Search::KING_SIDE_CASTLE_MOVE_MASK | Search::QUEEN_SIDE_CASTLE_MOVE_MASK))) {
                bestmove += Search::decodeBoardinv(resultMove.type, resultMove.to, resultMove.side);
                if (resultMove.promotionPiece != -1) {
                    bestmove += tolower(FEN_PIECE[(uchar) resultMove.promotionPiece]);
                }
            }

            if (abs(sc) > _INFINITE - MAX_PLY) {
                cout << "info score mate 1 depth " << mply << " nodes " << totMoves << " time " << timeTaken << " pv " << pvv << endl;
            } else {
                cout << "info score cp " << sc << " depth " << mply - extension << " nodes " << totMoves << " time " << timeTaken << " pv " << pvv << endl;
            }
        }

        if (searchManager.getForceCheck()) {
            searchManager.setForceCheck(false);
            searchManager.setRunning(1);

        } else if (abs(sc) > _INFINITE - MAX_PLY) {
            searchManager.setForceCheck(true);
            searchManager.setRunning(2);

        }
        if (mply >= maxDepth + extension && (searchManager.getRunning(0) != 2 || inMate)) {
            break;
        }

        if (abs(sc) > _INFINITE - MAX_PLY) {
            inMate = true;
        }
    }

    cout << "bestmove " << bestmove;
    if (ponderEnabled && ponderMove.size()) {
        cout << " ponder " << ponderMove;
    }
    cout << "\n" << flush;

}
