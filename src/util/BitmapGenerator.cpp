#include "BitmapGenerator.h"

u64 BitmapGenerator::ROTATE_BITMAP_DIAGONAL[64][256];
u64 BitmapGenerator::ROTATE_BITMAP_ANTIDIAGONAL[64][256];

BitmapGenerator::BitmapGenerator() {
    cout <<"BitmapGenerator..."<<endl;
    memset(ROTATE_BITMAP_DIAGONAL, -1, sizeof(ROTATE_BITMAP_DIAGONAL));
    memset(ROTATE_BITMAP_ANTIDIAGONAL, -1, sizeof(ROTATE_BITMAP_ANTIDIAGONAL));

    u64 key;
    Bits::getInstance();
    u64 i = 1;
    do {
        key = Random::getRandom64();
        if (!((i++) % 10000000))cout << dec << (i) << hex << " 0x" << key << "ULL\n";
    }
    while (!popolateDiagonal(key));
    cout << "TROVATO " << hex << key << endl;
    exit(0);
    popolateAntiDiagonal();
    cout << "aaa " << aaa << endl;
}

bool BitmapGenerator::popolateDiagonal(u64 key) {
    vector<u64> elements;

    for (uchar pos = 0; pos < 64; pos++) {
        elements.clear();
        u64 diag = _board::LEFT_DIAG[pos] | POW2[pos];
        while (diag) {
            int o = Bits::BITScanForward(diag);
            elements.push_back(o);
            RESET_LSB(diag);
        }
        vector<vector<u64>> res = getPermutation(elements);
        for (int i = 0; i < res.size(); i++) {
            u64 allpieces = 0;
            for (int y:res[i]) {
                _assert(y < 64);
                allpieces |= POW2[y];
            }

            uchar idx = diagonalIdx(pos, allpieces, key);
            ASSERT_RANGE(idx, 0, 255);

            u64 mapDiag = performDiagShift(pos, allpieces);
//            cout << "ROTATE_BITMAP_DIAGONAL[pos:0x" << hex << (int) pos << "][idx:0x" << (int) idx << "] <== " << "0x" << mapDiag << "ULL (allpieces: " << hex << "0x" << allpieces << "ULL)" << endl;
//            cout << "ROTATE_BITMAP_DIAGONAL valore precedente " << ROTATE_BITMAP_DIAGONAL[pos][idx] << endl;
            if (ROTATE_BITMAP_DIAGONAL[pos][idx] != mapDiag && ROTATE_BITMAP_DIAGONAL[pos][idx] != -1)
                return false;
            ROTATE_BITMAP_DIAGONAL[pos][idx] = mapDiag;
//            cout << "store ROTATE_BITMAP_DIAGONAL[pos:0x" << hex << (int) pos << "][idx:0x" << (int) idx << "]=" << "0x" << ROTATE_BITMAP_DIAGONAL[pos][idx] << endl;
        }
    }
    return true;
}

void BitmapGenerator::popolateAntiDiagonal() {

}


u64 BitmapGenerator::performDiagShift(const int position, const u64 allpieces) {
    /*
        LEFT
             /
            /
           /
*/
    u64 q = allpieces & MASK_BIT_UNSET_LEFT_UP[position];
    u64 k = q ? Bits::MASK_BIT_SET_NOBOUND[position][Bits::BITScanReverse(q)] : MASK_BIT_SET_LEFT_LOWER[position];
    q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];
    k |= q ? Bits::MASK_BIT_SET_NOBOUND[position][Bits::BITScanForward(q)] : MASK_BIT_SET_LEFT_UPPER[position];
    return k;

}

u64 BitmapGenerator::performAntiDiagShift(const int position, const u64 allpieces) {
    /*
        RIGHT
        \
         \
          \
*/
    u64 q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];
    u64 k = q ? Bits::MASK_BIT_SET_NOBOUND[position][Bits::BITScanReverse(q)] : MASK_BIT_SET_RIGHT_LOWER[position];
    q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];
    k |= q ? Bits::MASK_BIT_SET_NOBOUND[position][Bits::BITScanForward(q)] : MASK_BIT_SET_RIGHT_UPPER[position];
    return k;
}

vector<u64> BitmapGenerator::combinations_recursive(const vector<u64> &elems, unsigned long req_len,
                                                    vector<unsigned long> &pos, unsigned long depth,
                                                    unsigned long margin) {
    vector<u64> res;
    // Have we selected the number of required elements?
    if (depth >= req_len) {
        for (unsigned long ii = 0; ii < pos.size(); ++ii) {
            res.push_back(elems[pos[ii]]);
//            cout <<"X"<< elems[pos[ii]];
        }
//        cout << endl;
        return res;
    }

    // Are there enough remaining elements to be selected?
    // This test isn't required for the function to be correct, but
    // it can save a good amount of futile function calls.
    if ((elems.size() - margin) < (req_len - depth))
        return res;

    // Try to select new elements to the right of the last selected one.
    for (unsigned long ii = margin; ii < elems.size(); ++ii) {
        pos[depth] = ii;

        vector<u64> A = combinations_recursive(elems, req_len, pos, depth + 1, ii + 1);
        res.insert(res.end(), A.begin(), A.end());
    }

    return res;
}

vector<u64>  BitmapGenerator::combinations(const vector<u64> &elems, unsigned long comb_len) {
    assert(comb_len > 0 && comb_len <= elems.size());
    vector<unsigned long> positions(comb_len, 0);
    return combinations_recursive(elems, comb_len, positions, 0, 0);

}


vector<vector<u64>> BitmapGenerator::getPermutation(vector<u64> elements) {
    vector<vector<u64>> res(256);
    vector<u64> v;
    int nVector = 0;
    for (int comb_len = 1; comb_len < elements.size() + 1; comb_len++) {
        v = combinations(elements, comb_len);
        int k = 0;

        for (int rr:v) {
            //  cout << "(" << rr << ")";
            //  cout << nVector << endl;
            res[nVector].push_back(rr);
            if (++k == comb_len) {
                k = 0;
                nVector++;
                assert(nVector < 256);
//                res.resize(nVector);
                // cout << "\n";
            }
        }
        //  cout << "\n";
    }
    res.resize(nVector);
    return res;
}


