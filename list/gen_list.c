/*
 * SPDX-FileCopyrightText: 2010~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include <hangul.h>
#include <stdio.h>

int main() {
    unsigned int nkeyboard = hangul_ic_get_n_keyboards(), i;
    for (i = 0; i < nkeyboard; i++)
        printf("\"%s\" \"%s\"\n", hangul_ic_get_keyboard_name(i),
               hangul_ic_get_keyboard_id(i));
    return 0;
}
