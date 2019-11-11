//
// Copyright (C) 2010~2017 by CSSlayer
// wengxt@gmail.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <hangul.h>
#include <stdio.h>

int main() {
    unsigned int nkeyboard = hangul_ic_get_n_keyboards(), i;
    for (i = 0; i < nkeyboard; i++)
        printf("\"%s\" \"%s\"\n", hangul_ic_get_keyboard_name(i),
               hangul_ic_get_keyboard_id(i));
    return 0;
}
