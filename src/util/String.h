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

#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>
#include "String.h"


using namespace std;

class String : public string {
public:
    String();

    String(string s) : string(s) { };

    String(char *s) : string(s) { };

    String(int);

    String(unsigned long long);

    virtual ~ String();

    String trim();

    String trimLeft();

    String trimRight();

    String replace(string &s1, string &s2);

    String replace(char c1, char c2);

    String toUpper();

    String toLower();
};
