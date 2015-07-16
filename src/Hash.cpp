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

#include <mutex>
#include "Hash.h"

Hash::Hash() {
    HASH_SIZE = 0;
    hashArray[HASH_GREATER] = hashArray[HASH_ALWAYS] = nullptr;
#ifdef DEBUG_MODE
    n_cut_hashA = n_cut_hashE = n_cut_hashB = cutFailed = probeHash = 0;
    nRecordHashA = nRecordHashB = nRecordHashE = collisions = 0;
#endif
    setHashSize(64);
}

void Hash::clearAge() {
    for (int i = 0; i < HASH_SIZE; i++) {
        hashArray[HASH_GREATER][i].entryAge = 0;
    }
}

void Hash::clearHash() {
    if (!HASH_SIZE) {
        return;
    }
    memset(hashArray[HASH_ALWAYS], 0, sizeof(_Thash) * HASH_SIZE);
    memset(hashArray[HASH_GREATER], 0, sizeof(_Thash) * HASH_SIZE);
}

int Hash::getHashSize() {
    return HASH_SIZE / (1024 * 1000 / (sizeof(_Thash) * 2));
}

bool Hash::setHashSize(int mb) {
    if (mb < 1 || mb > 32768) {
        return false;
    }
    dispose();
//    double a=sizeof(mutexBucket)/1024.0/1024.0;
    if (mb) {
        HASH_SIZE = mb * 1024 * 1000 / (sizeof(_Thash) * 2);
        hashArray[HASH_GREATER] = (_Thash *) calloc(HASH_SIZE, sizeof(_Thash));
        assert(hashArray[HASH_GREATER]);
        hashArray[HASH_ALWAYS] = (_Thash *) calloc(HASH_SIZE, sizeof(_Thash));
        assert(hashArray[HASH_ALWAYS]);
    }
    return true;
}

bool Hash::readHash(_Thash *phashe[2], const int type, const u64 zobristKeyR, const int depth, _Thash &hashMini) {
    _Thash *hash = phashe[type] = &(hashArray[type][zobristKeyR % HASH_SIZE]);
    MUTEX_BUCKET[type][zobristKeyR % N_MUTEX_BUCKET].lock_shared();
    memcpy(&hashMini, hash, sizeof(_Thash));
    MUTEX_BUCKET[type][zobristKeyR % N_MUTEX_BUCKET].unlock_shared();
    if (hash->key == zobristKeyR && hash->depth >= depth && hash->from != hash->to) {
        return true;
    }
    return false;
}

void Hash::recordHash(u64 zobristKeyR, bool running, _Thash *phashe_greater, _Thash *phashe_always, const char depth, const char flags, const u64 key, const int score, _Tmove *bestMove) {
    ASSERT(key);
    ASSERT(phashe_greater);
    ASSERT(phashe_always);
    if (!running) {
        return;
    }
    ASSERT(abs(score) <= 32200);
    _Thash *phashe = phashe_greater;
    int keyMutex = zobristKeyR % N_MUTEX_BUCKET;
    MUTEX_BUCKET[HASH_GREATER][keyMutex].lock();

    phashe->key = key;
    phashe->score = score;
    phashe->flags = flags;
    phashe->depth = depth;
    if (bestMove && bestMove->from != bestMove->to) {
        phashe->from = bestMove->from;
        phashe->to = bestMove->to;
    } else {
        phashe->from = phashe->to = 0;
    }
    MUTEX_BUCKET[HASH_GREATER][keyMutex].unlock();
    phashe = phashe_always;
    MUTEX_BUCKET[HASH_ALWAYS][keyMutex].lock();
    if (phashe->key && phashe->depth >= depth && phashe->entryAge) {
        INC(collisions);
        MUTEX_BUCKET[HASH_ALWAYS][keyMutex].unlock();
        return;
    }
#ifdef DEBUG_MODE
    if (flags == hashfALPHA) {
        nRecordHashA++;
    } else if (flags == hashfBETA) {
        nRecordHashB++;
    } else {
        nRecordHashE++;
    }
#endif
    phashe->key = key;
    phashe->score = score;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->entryAge = 1;
    if (bestMove && bestMove->from != bestMove->to) {
        phashe->from = bestMove->from;
        phashe->to = bestMove->to;
    } else {
        phashe->from = phashe->to = 0;
    }
    MUTEX_BUCKET[HASH_ALWAYS][keyMutex].unlock();
}

void Hash::dispose() {
    if (hashArray[HASH_GREATER]) {
        free(hashArray[HASH_GREATER]);
    }
    if (hashArray[HASH_ALWAYS]) {
        free(hashArray[HASH_ALWAYS]);
    }
    hashArray[HASH_GREATER] = hashArray[HASH_ALWAYS] = nullptr;
    HASH_SIZE = 0;
}

Hash::~Hash() {
    dispose();
}


