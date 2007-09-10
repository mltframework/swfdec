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

ASSetPropFlags (Object, null, 7);

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

String = ASconstructor(251, 0);
ASSetNative(String.prototype, 251, "valueOf,toString,toUpperCase,toLowerCase,charAt,charCodeAt,concat,indexOf,lastIndexOf,slice,substring,split,substr", 1);
ASSetPropFlags(String.prototype, null, 3);
String.fromCharCode = ASnative(251, 14);
ASSetPropFlags(String, null, 3);

escape = ASnative (100, 0);
unescape = ASnative (100, 1);

Math = {
  E: 2.71828182845905,
  LN10: 2.30258509299405,
  LN2: 0.693147180559945,
  LOG10E: 0.434294481903252,
  LOG2E: 1.44269504088896,
  PI: 3.14159265358979,
  SQRT1_2: 0.707106781186548,
  SQRT2: 1.4142135623731
};
ASSetNative (Math, 200, "abs,min,max,sin,cos,atan2,tan,exp,log,sqrt,round,random,floor,ceil,atan,asin,acos,pow");
ASSetPropFlags (Math, null, 7);

Date = ASconstructor (103, 256);
ASSetNative (Date.prototype, 103, "getFullYear,getYear,getMonth,getDate,getDay,getHours,getMinutes,getSeconds,getMilliseconds,setFullYear,setMonth,setDate,setHours,setMinutes,setSeconds,setMilliseconds,getTime,setTime,getTimezoneOffset,toString,setYear");
ASSetNative (Date.prototype, 103, "getUTCFullYear,getUTCYear,getUTCMonth,getUTCDate,getUTCDay,getUTCHours,getUTCMinutes,getUTCSeconds,getUTCMilliseconds,setUTCFullYear,setUTCMonth,setUTCDate,setUTCHours,setUTCMinutes,setUTCSeconds,setUTCMilliseconds", 128);
Date.prototype.valueOf = Date.prototype.getTime;
Date.UTC = ASnative (103, 257);
ASSetPropFlags (Date.prototype, null, 3);
ASSetPropFlags (Date, null, 7);

/*** GLOBAL PROPFLAGS */

ASSetPropFlags (this, null, 1, 6);
