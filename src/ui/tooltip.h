/* SPDX-License-Identifier: MIT */
/* src/ui/tooltip.h - Inline tooltip bubble.
 *
 * KilnUI does not currently render Clay floating commands, so this tooltip is
 * an inline bubble. Place it near the hovered control in the layout.
 */

#ifndef UI_TOOLTIP_H
#define UI_TOOLTIP_H

#include <stdbool.h>

void UI_Tooltip(int uid, const char *text, bool visible);

#endif /* UI_TOOLTIP_H */
