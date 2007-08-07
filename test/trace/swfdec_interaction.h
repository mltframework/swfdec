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

#include <libswfdec/swfdec.h>
#include <glib.h>

#ifndef __SWFDEC_INTERACTION_H__
#define __SWFDEC_INTERACTION_H__


typedef enum {
  /* wait (int msecs) */
  SWFDEC_COMMAND_WAIT = G_TOKEN_LAST + 1,
  /* move (int x, int y) */
  SWFDEC_COMMAND_MOVE,
  /* mouse press (void) */
  SWFDEC_COMMAND_DOWN,
  /* mouse release (void) */
  SWFDEC_COMMAND_UP,
  /* key press (int key) */
  SWFDEC_COMMAND_PRESS,
  /* key release (int key) */
  SWFDEC_COMMAND_RELEASE
} SwfdecCommandType;

typedef struct _SwfdecCommand SwfdecCommand;
struct _SwfdecCommand {
  SwfdecCommandType		command;
  union {
    struct {
      int		x;
      int		y;
      int		button;
    }			mouse;
    guint		time;
    struct {
      SwfdecKey		code;
      guint		ascii;
    }			key;
  }			args;
};

typedef struct _SwfdecInteraction SwfdecInteraction;
struct _SwfdecInteraction {
  GArray *	commands;

  /* current state */
  int		mouse_x;
  int		mouse_y;
  int		mouse_button;
  /* current advancement state */
  guint		cur_idx;		/* current index into array (can be array->len if done) */
  guint		time_elapsed;		/* msecs elapsed in current wait event */
};

SwfdecInteraction *	swfdec_interaction_new			(const char *		data,
								 guint			length,
								 GError **		error);
SwfdecInteraction *	swfdec_interaction_new_from_file	(const char *		filename,
								 GError **		error);
void			swfdec_interaction_free			(SwfdecInteraction *	inter);
void			swfdec_interaction_reset		(SwfdecInteraction *	inter);

guint			swfdec_interaction_get_duration		(SwfdecInteraction *	inter);
int			swfdec_interaction_get_next_event	(SwfdecInteraction *	inter);
void			swfdec_interaction_advance		(SwfdecInteraction *	inter,
								 SwfdecPlayer *		player,
								 guint			msecs);


#endif /* __SWFDEC_INTERACTION_H__ */
