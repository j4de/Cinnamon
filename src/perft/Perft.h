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

#pragma once

#include "../Search.h"
#include <iomanip>
#include <atomic>
#include <fstream>
#include <unistd.h>
#include "../util/Timer.h"
#include "PerftThread.h"
#include "../threadPool/ThreadPool.h"
#include "_TPerftRes.h"
#include <signal.h>

class Perft : public Thread<Perft>, public ThreadPool<PerftThread>, public Singleton<Perft> {
    friend class Singleton<Perft>;

public:

    void setParam(string fen, int depth, int nCpu, int mbSize, string dumpFile);

    ~Perft();

    void dump();

    void run();

    void endRun();

    static int count;

    u64 getResult() {
        return perftRes.totMoves;
    }

private:
    Perft() : ThreadPool(1) { };

    _TPerftRes perftRes;
    high_resolution_clock::time_point start1;

    string fen;
    string dumpFile;
    u64 mbSize;

    void alloc();

    bool load();

    const static int minutesToDump = Time::HOUR_IN_MINUTES * 10;

    static void ctrlChandler(int s) {
        if (dumping) {
            cout << "dumping hash... " << endl;
            return;
        }

        Perft::getInstance().dump();
        if (s < 0)cout << s;
        exit(1);

    }

    static bool dumping;

};

