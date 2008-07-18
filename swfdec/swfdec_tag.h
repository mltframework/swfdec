/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef __SWFDEC_TAGS_H__
#define __SWFDEC_TAGS_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_TAG_END                  = 0,
  SWFDEC_TAG_SHOWFRAME            = 1,
  SWFDEC_TAG_DEFINESHAPE          = 2,
  SWFDEC_TAG_FREECHARACTER        = 3,
  SWFDEC_TAG_PLACEOBJECT          = 4,
  SWFDEC_TAG_REMOVEOBJECT         = 5,
  SWFDEC_TAG_DEFINEBITSJPEG       = 6,
  SWFDEC_TAG_DEFINEBUTTON         = 7,
  SWFDEC_TAG_JPEGTABLES           = 8,
  SWFDEC_TAG_SETBACKGROUNDCOLOR   = 9,
  SWFDEC_TAG_DEFINEFONT           = 10,
  SWFDEC_TAG_DEFINETEXT           = 11,
  SWFDEC_TAG_DOACTION             = 12,
  SWFDEC_TAG_DEFINEFONTINFO       = 13,
  SWFDEC_TAG_DEFINESOUND          = 14,      /* Event sound tags. */
  SWFDEC_TAG_STARTSOUND           = 15,
  SWFDEC_TAG_DEFINEBUTTONSOUND    = 17,
  SWFDEC_TAG_SOUNDSTREAMHEAD      = 18,
  SWFDEC_TAG_SOUNDSTREAMBLOCK     = 19,
  SWFDEC_TAG_DEFINEBITSLOSSLESS   = 20,      /* A bitmap using lossless zlib compression. */
  SWFDEC_TAG_DEFINEBITSJPEG2      = 21,      /* A bitmap using an internal JPEG compression table. */
  SWFDEC_TAG_DEFINESHAPE2         = 22,
  SWFDEC_TAG_DEFINEBUTTONCXFORM   = 23,
  SWFDEC_TAG_PROTECT              = 24,      /* This file should not be importable for editing. */
  SWFDEC_TAG_PLACEOBJECT2         = 26,      /* The new style place w/ alpha color transform and name. */
  SWFDEC_TAG_REMOVEOBJECT2        = 28,      /* A more compact remove object that omits the character tag (just depth). */
  SWFDEC_TAG_DEFINESHAPE3         = 32,      /* A shape V3 includes alpha values. */
  SWFDEC_TAG_DEFINETEXT2          = 33,      /* A text V2 includes alpha values. */
  SWFDEC_TAG_DEFINEBUTTON2        = 34,      /* A button V2 includes color transform, alpha and multiple actions */
  SWFDEC_TAG_DEFINEBITSJPEG3      = 35,      /* A JPEG bitmap with alpha info. */
  SWFDEC_TAG_DEFINEBITSLOSSLESS2  = 36,      /* A lossless bitmap with alpha info. */
  SWFDEC_TAG_DEFINEEDITTEXT       = 37,
  SWFDEC_TAG_DEFINEMOVIE          = 38,
  SWFDEC_TAG_DEFINESPRITE         = 39,      /* Define a sequence of tags that describe the behavior of a sprite. */
  SWFDEC_TAG_NAMECHARACTER        = 40,      /* Name a character definition, character id and a string, (used for buttons, bitmaps, sprites and sounds). */
  SWFDEC_TAG_PRODUCTINFO	  = 41,
  SWFDEC_TAG_GENERATORTEXT        = 42,      /* contains an id */
  SWFDEC_TAG_FRAMELABEL           = 43,      /* A string label for the current frame. */
  SWFDEC_TAG_SOUNDSTREAMHEAD2     = 45,      /* For lossless streaming sound, should not have needed this... */
  SWFDEC_TAG_DEFINEMORPHSHAPE     = 46,      /* A morph shape definition */
  SWFDEC_TAG_DEFINEFONT2          = 48,
  SWFDEC_TAG_TEMPLATECOMMAND      = 49,
  SWFDEC_TAG_GENERATOR3           = 51,
  SWFDEC_TAG_EXTERNALFONT         = 52,
  SWFDEC_TAG_EXPORTASSETS	  = 56,
  SWFDEC_TAG_IMPORTASSETS	  = 57,
  SWFDEC_TAG_ENABLEDEBUGGER	  = 58,
  SWFDEC_TAG_DOINITACTION	  = 59,
  SWFDEC_TAG_DEFINEVIDEOSTREAM	  = 60,
  SWFDEC_TAG_VIDEOFRAME		  = 61,
  SWFDEC_TAG_DEFINEFONTINFO2	  = 62,
  SWFDEC_TAG_DEBUGID		  = 63,
  SWFDEC_TAG_ENABLEDEBUGGER2	  = 64,
  SWFDEC_TAG_SCRIPTLIMITS	  = 65,
  SWFDEC_TAG_SETTABINDEX	  = 66,
#if 0
  /* magic tags that seem to be similar to FILEATTRIBUTES */
  SWFDEC_TAG_			  = 67,
  SWFDEC_TAG_			  = 68,
#endif
  SWFDEC_TAG_FILEATTRIBUTES	  = 69,
  SWFDEC_TAG_PLACEOBJECT3	  = 70,
  SWFDEC_TAG_IMPORTASSETS2	  = 71,
#if 0
  /* seems similar to SWFDEC_TAG_AVM2DECL */
  SWFDEC_TAG_			  = 72, /* allowed with DefineSprite */
#endif
  SWFDEC_TAG_DEFINEFONTALIGNZONES = 73,
  SWFDEC_TAG_CSMTEXTSETTINGS	  = 74,
  SWFDEC_TAG_DEFINEFONT3	  = 75,
  SWFDEC_TAG_AVM2DECL		  = 76,
  SWFDEC_TAG_METADATA		  = 77,
  SWFDEC_TAG_DEFINESCALINGGRID	  = 78,
#if 0
  /* more magic tags that seem to be similar to FILEATTRIBUTES */
  SWFDEC_TAG_			  = 80,
  SWFDEC_TAG_			  = 81,
#endif
  SWFDEC_TAG_AVM2ACTION		  = 82,
  SWFDEC_TAG_DEFINESHAPE4	  = 83,
  SWFDEC_TAG_DEFINEMORPHSHAPE2    = 84,
  SWFDEC_TAG_PRIVATE_IMAGE    	  = 85,
  SWFDEC_TAG_DEFINESCENEDATA  	  = 86,
  SWFDEC_TAG_DEFINEBINARYDATA 	  = 87,
  SWFDEC_TAG_DEFINEFONTNAME	  = 88,
  SWFDEC_TAG_STARTSOUND2	  = 89
} SwfdecTag;

typedef enum {
  /* tag is allowe inside DefineSprite */
  SWFDEC_TAG_DEFINE_SPRITE = (1 << 0),
  /* tag must be first tag */
  SWFDEC_TAG_FIRST_ONLY = (1 << 1)
} SwfdecTagFlag;

G_END_DECLS
#endif
