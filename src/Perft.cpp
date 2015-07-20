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

#include "Perft.h"

mutex updateHash;

Perft::PerftThread::PerftThread() { }

Perft::PerftThread::PerftThread(int cpuID1, string fen1, int from1, int to1, Perft *perft1) : GenMoves() {
    perftMode = true;
    loadFen(fen1);
    this->cpuID = cpuID1;
    this->perft = perft1;
    this->from = from1;
    this->to = to1;
}

void Perft::dump() {
    if (dumpFile.empty() || !hash) {
        return;
    }

    cout << endl << "Dump hash table in " << dumpFile << " file..." << flush;
    ofstream f;
    string tmpFile = dumpFile + ".tmp";
    f.open(tmpFile, ios_base::out | ios_base::binary);
    if (!f.is_open()) {
        cout << "error create file " << tmpFile << endl;
        return;
    }
    for (auto it:threadList) {
        it->sleep(true);
    }
    f << fen;
    f.put(10);
    f.write(reinterpret_cast<char *>(&depth), sizeof(int));
    f.write(reinterpret_cast<char *>(&nCpu), sizeof(int));
    f.write(reinterpret_cast<char *>(&mbSize), sizeof(u64));
    for (int i = 1; i <= depth; i++) {
        f.write(reinterpret_cast<char *>(hash[i]), sizeAtDepth[i] * sizeof(_ThashPerft));
    }
    f.close();
    rename(tmpFile.c_str(), dumpFile.c_str());
    cout << "ok" << endl;
    for (auto it:threadList) {
        it->sleep(false);
        it->notify();
    }
}

bool Perft::load() {
    if (dumpFile.empty()) {
        return false;
    }
    ifstream f;
    string fen1;
    int nCpuHash, depthHash;
    u64 mbSizeHash;
    if (!_file::fileExists(dumpFile)) {
        return false;
    }
    f.open(dumpFile, ios_base::in | ios_base::binary);
    cout << endl << "load hash table from " << dumpFile << " file.." << endl;
    getline(f, fen1);
    f.read(reinterpret_cast<char *>(&depthHash), sizeof(int));
    if (depthHash > depth) {
        cout << "error depth < hash depth" << endl;
        f.close();
        exit(1);
    };
    f.read(reinterpret_cast<char *>(&nCpuHash), sizeof(int));
    f.read(reinterpret_cast<char *>(&mbSizeHash), sizeof(u64));
    alloc();
    if (fen.empty()) {
        fen = fen1;
    }
    if (!nCpu) {
        nCpu = nCpuHash;
    }
    cout << " fen: " << fen << "\n";
    cout << " mbSize: " << mbSize << "\n";
    cout << " depth: " << depth << "\n";
    cout << " nCpu: " << nCpu << "\n";
    totMoves = 0;
    u64 kHash = 1024 * 1024 * mbSizeHash / POW2[depthHash];
    u64 sizeAtDepthHash[255];
    for (int i = 1; i <= depthHash; i++) {
        sizeAtDepthHash[i] = kHash * POW2[i - 1] / sizeof(_ThashPerft);
        cout << sizeAtDepthHash[i] * sizeof(_ThashPerft) << "\n";
    }
    _ThashPerft *tmp = (_ThashPerft *) malloc(sizeAtDepthHash[depthHash] * sizeof(_ThashPerft));
    assert(tmp);
    for (int i = 1; i <= depthHash; i++) {
        f.read(reinterpret_cast<char *>(tmp), sizeAtDepthHash[i] * sizeof(_ThashPerft));
        for (unsigned y = 0; y < sizeAtDepthHash[i]; y++) {
            if (tmp[y].key) {
                u64 rr = tmp[y].key % sizeAtDepth[i];
                hash[i][rr].key = tmp[y].key;
                hash[i][rr].nMoves = tmp[y].nMoves;
            }
        }
    }
    free(tmp);
    f.close();
    cout << "loaded" << endl;
    return true;
}

Perft::~Perft() {
    if (timer) {
        delete timer;
    }
    if (hash) {
        for (int i = 1; i <= depth; i++) {
            free(hash[i]);
        }
        free(hash);
    }
}

void Perft::alloc() {
    hash = (_ThashPerft **) calloc(depth + 1, sizeof(_ThashPerft * ));
    assert(hash);
    u64 k = 1024 * 1024 * mbSize / POW2[depth];
    for (int i = 1; i <= depth; i++) {
        sizeAtDepth[i] = k * POW2[i - 1] / sizeof(_ThashPerft);
        hash[i] = (_ThashPerft *) calloc(sizeAtDepth[i], sizeof(_ThashPerft));
#ifdef DEBUG_MODE
        cout << "alloc hash[" << i << "] " << sizeAtDepth[i] * sizeof(_ThashPerft) << endl;
#endif
        assert(hash[i]);
    }
}

Perft::Perft(string fen1, int depth1, int nCpu2, int mbSize1, string dumpFile1) {
    mbSize = mbSize1;
    depth = depth1;
    fen = fen1;
    nCpu = nCpu2;
    dumpFile = dumpFile1;
    totMoves = 0;
}

void Perft::run() {

    if (!load()) {
        hash = nullptr;
        if (mbSize) {
            alloc();
        }
    }
    if (hash && !dumpFile.empty()) {
        timer = new Timer(secondsToDump);
        cout << "dump hash table in file every " << (secondsToDump / 60) << " minutes or type 'flush'" << endl;
        timer->registerObservers([this]() {
            dump();
        });
        timer->start();
    }
    if (fen.empty()) {
        fen = STARTPOS;
    }
    if (!depth) {
        depth = 1;
    }
    if (!nCpu) {
        nCpu = 1;
    }
    PerftThread *p = new PerftThread();
    if (!fen.empty()) {
        p->loadFen(fen);
    }
    p->setPerft(true);
    int side = p->getSide() ? 1 : 0;
    p->display();
    cout << "fen:\t\t\t" << fen << "\n";
    cout << "depth:\t\t\t" << depth << "\n";
    cout << "#cpu:\t\t\t" << nCpu << "\n";
    cout << "cache size:\t\t" << mbSize << "\n";
    cout << "dump file:\t\t" << dumpFile << "\n";
    cout << "\nstart...\n";
    struct timeb start1, end1;
    ftime(&start1);
    p->incListId();
    u64 friends = side ? p->getBitBoard<WHITE>() : p->getBitBoard<BLACK>();
    u64 enemies = side ? p->getBitBoard<BLACK>() : p->getBitBoard<WHITE>();
    p->generateCaptures(side, enemies, friends);
    p->generateMoves(side, friends | enemies);
    int listcount = p->getListSize();
    delete(p);
    p = nullptr;
    ASSERT(nCpu > 0);
    int block = listcount / nCpu;
    int i, s = 0;
    for (i = 0; i < nCpu - 1; i++) {
        threadList.push_back(new PerftThread(i, fen, s, s + block, this));
        s += block;
    }
    threadList.push_back(new PerftThread(i, fen, s, listcount, this));
    for (auto it:threadList) {
        it->start();
    }
    for (auto it:threadList) {
        it->join();
    }
    ftime(&end1);
    int t = _time::diffTime(end1, start1) / 1000;
    int days = t / 60 / 60 / 24;
    int hours = (t / 60 / 60) % 24;
    int minutes = (t / 60) % 60;
    int seconds = t % 60;
    cout << endl << endl << "Perft moves: " << totMoves << " in ";
    if (days) {
        cout << days << " days, ";
    }
    if (days || hours) {
        cout << hours << " hours, ";
    }
    if (days || hours || minutes) {
        cout << minutes << " minutes, ";
    }
    if (!days) {
        cout << seconds << " seconds";
    }
    if (t) {
        cout << " (" << (totMoves / t) / 1000 - ((totMoves / t) / 1000) % 1000 << "k nodes per seconds" << ")";
    }
    cout << endl;
    dump();
    for (auto it:threadList) {
        delete it;
        it = nullptr;
    }
    threadList.clear();
    notifyObservers();
}

template<int side, bool useHash>
u64 Perft::PerftThread::search(const int depthx) {
    checkWait();
    if (depthx == 0) {
        return 1;
    }
    u64 zobristKeyR;
    u64 n_perft = 0;
    _ThashPerft *phashe = nullptr;
    if (useHash) {
        zobristKeyR = chessboard[ZOBRISTKEY_IDX] ^ RANDSIDE[side];
        lock_guard<mutex> lock(perft->updateHash);
        phashe = &(perft->hash[depthx][zobristKeyR % perft->sizeAtDepth[depthx]]);
        if (zobristKeyR == phashe->key) {
            n_perft = phashe->nMoves;
            return n_perft;
        }
    }
    int listcount;
    _Tmove *move;
    incListId();
    u64 friends = getBitBoard<side>();
    u64 enemies = getBitBoard<side ^ 1>();
    if (generateCaptures<side>(enemies, friends)) {
        decListId();
        return 0;
    }
    generateMoves<side>(friends | enemies);
    listcount = getListSize();
    if (!listcount) {
        decListId();
        return 0;
    }
    for (int ii = 0; ii < listcount; ii++) {
        move = getMove(ii);
        u64 keyold = chessboard[ZOBRISTKEY_IDX];
        makemove(move, false, false);
        n_perft += search<side ^ 1, useHash>(depthx - 1);
        takeback(move, keyold, false);
    }
    decListId();
    if (useHash) {
        lock_guard<mutex> lock(perft->updateHash);
        phashe->nMoves = n_perft;
        phashe->key = zobristKeyR;
    }
    return n_perft;
}

void Perft::PerftThread::run() {
    init();
    _Tmove *move;
    incListId();
    resetList();
    u64 friends = chessboard[SIDETOMOVE_IDX] ? getBitBoard<WHITE>() : getBitBoard<BLACK>();
    u64 enemies = chessboard[SIDETOMOVE_IDX] ? getBitBoard<BLACK>() : getBitBoard<WHITE>();
    generateCaptures(chessboard[SIDETOMOVE_IDX], enemies, friends);
    generateMoves(chessboard[SIDETOMOVE_IDX], friends | enemies);
    u64 tot = 0;
    makeZobristKey();
    u64 keyold = chessboard[ZOBRISTKEY_IDX];
    for (int ii = to - 1; ii >= from; ii--) {
        u64 n_perft = 0;
        move = getMove(ii);
        makemove(move, false, false);
        if (perft->hash != nullptr) {
            n_perft = (chessboard[SIDETOMOVE_IDX] ^ 1) == WHITE ? search<WHITE, true>(perft->depth - 1) : search<BLACK, true>(perft->depth - 1);
        } else {
            n_perft = (chessboard[SIDETOMOVE_IDX] ^ 1) == WHITE ? search<WHITE, false>(perft->depth - 1) : search<BLACK, false>(perft->depth - 1);
        }
        takeback(move, keyold, false);
        char y;
        char x = FEN_PIECE[chessboard[SIDETOMOVE_IDX] ? getPieceAt<WHITE>(POW2[move->from]) : getPieceAt<BLACK>(POW2[move->from])];
        if (x == 'p' || x == 'P') {
            x = ' ';
        }
        if (move->capturedPiece != SQUARE_FREE) {
            y = '*';
        } else {
            y = '-';
        }
        {
            lock_guard<mutex> lock(perft->mutexPrint);
            cout << endl << "#" << ii + 1 << " cpuID# " << cpuID;
            if ((decodeBoardinv(move->type, move->to, chessboard[SIDETOMOVE_IDX])).length() > 2) {
                cout << "\t" << decodeBoardinv(move->type, move->to, chessboard[SIDETOMOVE_IDX]) << "\t" << n_perft << " ";
            } else {
                cout << "\t" << x << decodeBoardinv(move->type, move->from, chessboard[SIDETOMOVE_IDX]) << y << decodeBoardinv(move->type, move->to, chessboard[SIDETOMOVE_IDX]) << "\t" << n_perft << " ";
            }
        }
        cout << flush;
        tot += n_perft;
    }
    decListId();
    perft->setResult(tot);
}

Perft::PerftThread::~PerftThread() {
}

void Perft::registerObservers(function<void(void)> f) {
    observers.push_back(f);
}

void Perft::notifyObservers(void) {
    for (auto i = observers.begin(); i != observers.end(); ++i) {
        (*i)();
    }
}