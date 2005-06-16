/* tags.h */
/* Take from: */
/* rfxswf.h

   Headers for rfxswf.c and modules

   Part of the swftools package.

   Copyright (c) 2000, 2001 Rainer Böhme <rfxswf@reflex-studio.de>
 
   This file is distributed under the GPL, see file COPYING for details 

*/

#ifndef __LIBSWFDECODE_TAGS_H__
#define __LIBSWFDECODE_TAGS_H__

/* Tag IDs (adopted from J. C. Kessels' Form2Flash) */

#define ST_END                  0
#define ST_SHOWFRAME            1
#define ST_DEFINESHAPE          2
#define ST_FREECHARACTER        3
#define ST_PLACEOBJECT          4
#define ST_REMOVEOBJECT         5
#define ST_DEFINEBITS           6
#define ST_DEFINEBITSJPEG       6
#define ST_DEFINEBUTTON         7
#define ST_JPEGTABLES           8
#define ST_SETBACKGROUNDCOLOR   9
#define ST_DEFINEFONT           10
#define ST_DEFINETEXT           11
#define ST_DOACTION             12
#define ST_DEFINEFONTINFO       13
#define ST_DEFINESOUND          14      /* Event sound tags. */
#define ST_STARTSOUND           15
#define ST_DEFINEBUTTONSOUND    17
#define ST_SOUNDSTREAMHEAD      18
#define ST_SOUNDSTREAMBLOCK     19
#define ST_DEFINEBITSLOSSLESS   20      /* A bitmap using lossless zlib compression. */
#define ST_DEFINEBITSJPEG2      21      /* A bitmap using an internal JPEG compression table. */
#define ST_DEFINESHAPE2         22
#define ST_DEFINEBUTTONCXFORM   23
#define ST_PROTECT              24      /* This file should not be importable for editing. */
#define ST_PLACEOBJECT2         26      /* The new style place w/ alpha color transform and name. */
#define ST_REMOVEOBJECT2        28      /* A more compact remove object that omits the character tag (just depth). */
#define ST_DEFINESHAPE3         32      /* A shape V3 includes alpha values. */
#define ST_DEFINETEXT2          33      /* A text V2 includes alpha values. */
#define ST_DEFINEBUTTON2        34      /* A button V2 includes color transform, alpha and multiple actions */
#define ST_DEFINEBITSJPEG3      35      /* A JPEG bitmap with alpha info. */
#define ST_DEFINEBITSLOSSLESS2  36      /* A lossless bitmap with alpha info. */
#define ST_DEFINEEDITTEXT       37
#define ST_DEFINEMOVIE          38
#define ST_DEFINESPRITE         39      /* Define a sequence of tags that describe the behavior of a sprite. */
#define ST_NAMECHARACTER        40      /* Name a character definition, character id and a string, (used for buttons, bitmaps, sprites and sounds). */
#define ST_SERIALNUMBER         41
#define ST_GENERATORTEXT        42      /* contains an id */
#define ST_FRAMELABEL           43      /* A string label for the current frame. */
#define ST_SOUNDSTREAMHEAD2     45      /* For lossless streaming sound, should not have needed this... */
#define ST_DEFINEMORPHSHAPE     46      /* A morph shape definition */
#define ST_DEFINEFONT2          48
#define ST_TEMPLATECOMMAND      49
#define ST_GENERATOR3           51
#define ST_EXTERNALFONT         52
#define ST_EXPORTASSETS		56
#define ST_IMPORTASSETS		57
#define ST_ENABLEDEBUGGER	58
#define ST_DOINITACTION		59
#define ST_DEFINEVIDEOSTREAM	60
#define ST_VIDEOFRAME		61
#define ST_DEFINEFONTINFO2	62
#define ST_MX4			63      /*(?) */
#define ST_ENABLEDEBUGGER2	64
#define ST_SCRIPTLIMITS		65
#define ST_SETTABINDEX		66

#define ST_REFLEX              777      /* to identify generator software */


#endif
