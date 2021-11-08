/*
 * SPDX-FileCopyrightText: 2010~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
