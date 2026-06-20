/* SPDX-License-Identifier: MIT */
/* src/ui/dropdown.h - Dropdown menu shell.
 *
 * The caller owns open/closed state. Toggle it when header_clicked is true,
 * and apply selected_index when it is >= 0.
 */

#ifndef UI_DROPDOWN_H
#define UI_DROPDOWN_H

#include <stdbool.h>

typedef struct
{
    bool header_clicked;
    int selected_index;
} UIDropdownResult;

UIDropdownResult UI_Dropdown(int uid, const char *label,
                             const char *const *items, int item_count,
                             int selected_index, bool open, bool disabled);

#endif /* UI_DROPDOWN_H */
