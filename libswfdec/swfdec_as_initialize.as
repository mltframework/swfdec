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

/*** GENERAL ***/

ASSetNative = ASnative (4, 0);
ASSetNativeAccessor = ASnative (4, 1);

/*** BASE OBJECTS ***/

Boolean = ASconstructor(107, 2);
ASSetNative(Boolean.prototype, 107, "valueOf,toString");
ASSetPropFlags(Boolean.prototype, null, 3);

Number = ASconstructor (106, 2);
ASSetNative (Number.prototype, 106, "valueOf,toString");
ASSetPropFlags(Number.prototype, null, 3);
Number.NaN = NaN;
Number.POSITIVE_INFINITY = Infinity;
Number.NEGATIVE_INFINITY = -Infinity;
Number.MIN_VALUE = 0;
Number.MAX_VALUE = 1.79769313486231e+308;
ASSetPropFlags(Number, null, 7);
