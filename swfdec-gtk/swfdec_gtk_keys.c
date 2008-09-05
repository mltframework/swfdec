/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "swfdec_gtk_keys.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

static const guint8 flash_keycodes_old[] = {
  /*   0 */ 0, 0, 0, 0, 0, 
  /*   5 */ 0, 0, 0, 0, SWFDEC_KEY_ESCAPE,
  /*  10 */ SWFDEC_KEY_1, SWFDEC_KEY_2, SWFDEC_KEY_3, SWFDEC_KEY_4, SWFDEC_KEY_5, 
  /*  15 */ SWFDEC_KEY_6, SWFDEC_KEY_7, SWFDEC_KEY_8, SWFDEC_KEY_9, SWFDEC_KEY_0,
  /*  20 */ SWFDEC_KEY_MINUS, SWFDEC_KEY_EQUAL, SWFDEC_KEY_BACKSPACE, SWFDEC_KEY_TAB, SWFDEC_KEY_Q,
  /*  25 */ SWFDEC_KEY_W, SWFDEC_KEY_E, SWFDEC_KEY_R, SWFDEC_KEY_T, SWFDEC_KEY_Y,
  /*  30 */ SWFDEC_KEY_U, SWFDEC_KEY_I, SWFDEC_KEY_O, SWFDEC_KEY_P, SWFDEC_KEY_LEFT_BRACKET,
  /*  35 */ SWFDEC_KEY_RIGHT_BRACKET, SWFDEC_KEY_ENTER, SWFDEC_KEY_CONTROL, SWFDEC_KEY_A, SWFDEC_KEY_S,
  /*  40 */ SWFDEC_KEY_D, SWFDEC_KEY_F, SWFDEC_KEY_G, SWFDEC_KEY_H, SWFDEC_KEY_J,
  /*  45 */ SWFDEC_KEY_K, SWFDEC_KEY_L, SWFDEC_KEY_SEMICOLON, SWFDEC_KEY_APOSTROPHE, SWFDEC_KEY_GRAVE,
  /*  50 */ SWFDEC_KEY_SHIFT, SWFDEC_KEY_BACKSLASH, SWFDEC_KEY_Z, SWFDEC_KEY_X, SWFDEC_KEY_C, 
  /*  55 */ SWFDEC_KEY_V, SWFDEC_KEY_B, SWFDEC_KEY_N, SWFDEC_KEY_M, SWFDEC_KEY_COMMA, 
  /*  60 */ SWFDEC_KEY_DOT, SWFDEC_KEY_SLASH, SWFDEC_KEY_SHIFT, SWFDEC_KEY_NUMPAD_MULTIPLY, SWFDEC_KEY_ALT,
  /*  65 */ SWFDEC_KEY_SPACE, SWFDEC_KEY_CAPS_LOCK, SWFDEC_KEY_F1, SWFDEC_KEY_F2, SWFDEC_KEY_F3, 
  /*  70 */ SWFDEC_KEY_F4, SWFDEC_KEY_F5, SWFDEC_KEY_F6, SWFDEC_KEY_F7, SWFDEC_KEY_F8,
  /*  75 */ SWFDEC_KEY_F9, SWFDEC_KEY_F10, SWFDEC_KEY_NUM_LOCK, SWFDEC_KEY_SCROLL_LOCK, SWFDEC_KEY_NUMPAD_7,
  /*  80 */ SWFDEC_KEY_NUMPAD_8, SWFDEC_KEY_NUMPAD_9, SWFDEC_KEY_NUMPAD_SUBTRACT, SWFDEC_KEY_NUMPAD_4, SWFDEC_KEY_NUMPAD_5,
  /*  85 */ SWFDEC_KEY_NUMPAD_6, SWFDEC_KEY_NUMPAD_ADD, SWFDEC_KEY_NUMPAD_1, SWFDEC_KEY_NUMPAD_2, SWFDEC_KEY_NUMPAD_3,
  /*  90 */ SWFDEC_KEY_NUMPAD_0, SWFDEC_KEY_NUMPAD_DECIMAL, 0, 0, 0,
  /*  95 */ SWFDEC_KEY_F11, SWFDEC_KEY_F12, SWFDEC_KEY_HOME, SWFDEC_KEY_UP, SWFDEC_KEY_PAGE_UP,
  /* 100 */ SWFDEC_KEY_LEFT, 0, SWFDEC_KEY_RIGHT, SWFDEC_KEY_END, SWFDEC_KEY_DOWN,
  /* 105 */ SWFDEC_KEY_PAGE_DOWN, SWFDEC_KEY_INSERT, SWFDEC_KEY_DELETE, SWFDEC_KEY_ENTER, SWFDEC_KEY_CONTROL,
  /* 110 */ 0, 0, SWFDEC_KEY_NUMPAD_DIVIDE, SWFDEC_KEY_ALT, 0,
  /* 115 */ 0, 0, 0, 0, 0
};
/*
SWFDEC_KEY_BREAK 110
SWFDEC_KEY_META 115
SWFDEC_KEY_META_R 116
SWFDEC_KEY_MENU 117
*/
static const guint8 flash_keycodes_evdev[] = {
  /*   0 */ 0, 0, 0, 0, 0,
  /*   5 */ 0, 0, 0, 0, SWFDEC_KEY_ESCAPE,
  /*  10 */ SWFDEC_KEY_1, SWFDEC_KEY_2, SWFDEC_KEY_3, SWFDEC_KEY_4, SWFDEC_KEY_5, 
  /*  15 */ SWFDEC_KEY_6, SWFDEC_KEY_7, SWFDEC_KEY_8, SWFDEC_KEY_9, SWFDEC_KEY_0,
  /*  20 */ SWFDEC_KEY_MINUS, SWFDEC_KEY_EQUAL, SWFDEC_KEY_BACKSPACE, SWFDEC_KEY_TAB, SWFDEC_KEY_Q,
  /*  25 */ SWFDEC_KEY_W, SWFDEC_KEY_E, SWFDEC_KEY_R, SWFDEC_KEY_T, SWFDEC_KEY_Y,
  /*  30 */ SWFDEC_KEY_U, SWFDEC_KEY_I, SWFDEC_KEY_O, SWFDEC_KEY_P, SWFDEC_KEY_LEFT_BRACKET,
  /*  35 */ SWFDEC_KEY_RIGHT_BRACKET, SWFDEC_KEY_ENTER, SWFDEC_KEY_CONTROL, SWFDEC_KEY_A, SWFDEC_KEY_S,
  /*  40 */ SWFDEC_KEY_D, SWFDEC_KEY_F, SWFDEC_KEY_G, SWFDEC_KEY_H, SWFDEC_KEY_J,
  /*  45 */ SWFDEC_KEY_K, SWFDEC_KEY_L, SWFDEC_KEY_SEMICOLON, SWFDEC_KEY_APOSTROPHE, SWFDEC_KEY_GRAVE,
  /*  50 */ SWFDEC_KEY_SHIFT, SWFDEC_KEY_BACKSLASH, SWFDEC_KEY_Z, SWFDEC_KEY_X, SWFDEC_KEY_C, 
  /*  55 */ SWFDEC_KEY_V, SWFDEC_KEY_B, SWFDEC_KEY_N, SWFDEC_KEY_M, SWFDEC_KEY_COMMA, 
  /*  60 */ SWFDEC_KEY_DOT, SWFDEC_KEY_SLASH, SWFDEC_KEY_SHIFT, SWFDEC_KEY_NUMPAD_MULTIPLY, SWFDEC_KEY_ALT,
  /*  65 */ SWFDEC_KEY_SPACE, SWFDEC_KEY_CAPS_LOCK, SWFDEC_KEY_F1, SWFDEC_KEY_F2, SWFDEC_KEY_F3, 
  /*  70 */ SWFDEC_KEY_F4, SWFDEC_KEY_F5, SWFDEC_KEY_F6, SWFDEC_KEY_F7, SWFDEC_KEY_F8,
  /*  75 */ SWFDEC_KEY_F9, SWFDEC_KEY_F10, SWFDEC_KEY_NUM_LOCK, SWFDEC_KEY_SCROLL_LOCK, SWFDEC_KEY_NUMPAD_7,
  /*  80 */ SWFDEC_KEY_NUMPAD_8, SWFDEC_KEY_NUMPAD_9, SWFDEC_KEY_NUMPAD_SUBTRACT, SWFDEC_KEY_NUMPAD_4, SWFDEC_KEY_NUMPAD_5,
  /*  85 */ SWFDEC_KEY_NUMPAD_6, SWFDEC_KEY_NUMPAD_ADD, SWFDEC_KEY_NUMPAD_1, SWFDEC_KEY_NUMPAD_2, SWFDEC_KEY_NUMPAD_3,
  /*  90 */ SWFDEC_KEY_NUMPAD_0, SWFDEC_KEY_NUMPAD_DECIMAL, 0, 0, 0,
  /*  95 */ SWFDEC_KEY_F11, SWFDEC_KEY_F12, 0, 0, 0,
  /* 100 */ 0, 0, 0, 0, 0,
  /* 105 */ SWFDEC_KEY_CONTROL, SWFDEC_KEY_NUMPAD_DIVIDE, 0, SWFDEC_KEY_ALT, 0,
  /* 110 */ SWFDEC_KEY_HOME, SWFDEC_KEY_UP, SWFDEC_KEY_PAGE_UP, SWFDEC_KEY_LEFT, SWFDEC_KEY_RIGHT,
  /* 115 */ SWFDEC_KEY_END, SWFDEC_KEY_DOWN, SWFDEC_KEY_PAGE_DOWN, SWFDEC_KEY_INSERT, SWFDEC_KEY_DELETE
};

static const guint8 *
compute_keycodes (void)
{
  /* This is a huge hack, but a proper solution is probably very complicated
   * and I'm lazy:
   * We need a proper way to convert X keycodes to Windows virtual keycodes
   * and according to the people in #xorg-devel noone else has succeeded there
   * so far. */
  static const GdkKeymapKey test = { 105, 0, 0 };
  guint keyval = gdk_keymap_lookup_key (gdk_keymap_get_default (), &test);
  
  /* sanity check */
  g_assert (G_N_ELEMENTS (flash_keycodes_old) == G_N_ELEMENTS (flash_keycodes_evdev));

  if (keyval == GDK_Page_Down) {
    return flash_keycodes_old;
  } else if (keyval != GDK_Control_R) {
    g_printerr ("FIXME: Failed to detect proper keyboard layout, assuming evdev.\n");
  }

  return flash_keycodes_evdev;
}

/**
 * swfdec_gtk_keycode_from_hardware_keycode:
 * @hardware_keycode: a hardware keycode sent from the X server
 *
 * Tries to transform an X hardware keycode to the corresponding #SwfdecKey.
 * This is a utility function for cases where key events need to be transformed
 * manually.
 *
 * Returns: the corresponding key code as used in Flash.
 **/
guint
swfdec_gtk_keycode_from_hardware_keycode (guint hardware_keycode)
{
  static const guint8 *keycodes = NULL;

  if (G_UNLIKELY (keycodes == NULL))
    keycodes = compute_keycodes ();

  if (hardware_keycode >= G_N_ELEMENTS (flash_keycodes_evdev))
    return 0;
  return keycodes[hardware_keycode];
}
