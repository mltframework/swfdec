/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_version.h"


/**
 * SECTION:Version
 * @title: Version Information
 * @short_description: Compile-time and run-time version checks
 *
 * Swfdec has a three-part version number scheme. In this scheme, we use
 * even vs. odd numbers to distinguish fixed points in the software vs. 
 * in-progress development, (such as from git instead of a tar file, or as a 
 * "snapshot" tar file as opposed to a "release" tar file).
 *
 * <informalexample><programlisting>
 *  _____ Major. Always 1, until we invent a new scheme.
 * /  ___ Minor. Even/Odd = Release/Snapshot (tar files) or Branch/Head (git)
 * | /  _ Micro. Even/Odd = Tar-file/git
 * | | /
 * 1.0.0
 * </programlisting></informalexample>
 *
 * Here are a few examples of versions that one might see.
 * <informalexample><programlisting>
 * Releases
 * --------
 *  1.0.0 - A major release
 *  1.0.2 - A subsequent maintenance release
 *  1.2.0 - Another major release
 *
 * Snapshots
 * ---------
 *  1.1.2 - A snapshot (working toward the 1.2.0 release)
 *
 * In-progress development (eg. from git)
 * --------------------------------------
 *  1.0.1 - Development on a maintenance branch (toward 1.0.2 release)
 *  1.1.1 - Development on head (toward 1.1.2 snapshot and 1.2.0 release)
 *  </programlisting></informalexample>
 *
 * <refsect2><title>Examining the version</title><para>
 * Swfdec provides the ability to examine the version at either 
 * compile-time or run-time and in both a human-readable form as well as an 
 * encoded form suitable for direct comparison. Swfdec also provides the macro 
 * SWFDEC_VERSION_ENCODE() to perform the encoding.
 *
 * <informalexample><programlisting>
 * Compile-time
 * ------------
 *  SWFDEC_VERSION_STRING	Human-readable
 *  SWFDEC_VERSION		Encoded, suitable for comparison
 *
 * Run-time
 * --------
 *  swfdec_version_string()	Human-readable
 *  swfdec_version()		Encoded, suitable for comparison
 * </programlisting></informalexample>
 *  
 * For example, checking that the Swfdec version is greater than or equal
 * to 1.0.0 could be achieved at compile-time or run-time as follows:
 * <informalexample><programlisting>
 * #if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 0, 0)
 * printf ("Compiling with suitable cairo version: %s\n", %CAIRO_VERSION_STRING);
 * #endif
 *
 * if (cairo_version() >= CAIRO_VERSION_ENCODE(1, 0, 0))
 *   printf ("Running with suitable cairo version: %s\n", cairo_version_string ());
 * </programlisting></informalexample>
 * </para></refsect2>
 */
/**
 * SWFDEC_VERSION:
 *
 * The version of Swfdec available at compile-time, encoded using 
 * SWFDEC_VERSION_ENCODE(). 
 */
/**
 * SWFDEC_VERSION_MAJOR:
 *
 * The major component of the version of Swfdec available at compile-time.
 */
/**
 * SWFDEC_VERSION_MINOR:
 *
 * The minor component of the version of Swfdec available at compile-time.
 */
/**
 * SWFDEC_VERSION_MICRO:
 *
 * The micro component of the version of Swfdec available at compile-time.
 */
/**
 * SWFDEC_VERSION_STRING:
 *
 * A human-readable string literal containing the version of Swfdec available 
 * at compile-time, in the form of "X.Y.Z".
 */
/**
 * SWFDEC_VERSION_ENCODE:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number 
 *
 * This macro encodes the given Swfdec version into an integer. The numbers 
 * returned by @SWFDEC_VERSION and swfdec_version() are encoded using this 
 * macro. Two encoded version numbers can be compared as integers. The 
 * encoding ensures that later versions compare greater than earlier versions.
 */

#define SWFDEC_VERSION SWFDEC_VERSION_ENCODE (SWFDEC_VERSION_MAJOR, SWFDEC_VERSION_MINOR, SWFDEC_VERSION_MICRO)

/**
 * swfdec_version:
 *
 * Returns the version of the Swfdec library encoded in a single
 * integer as per %SWFDEC_VERSION_ENCODE. The encoding ensures that
 * later versions compare greater than earlier versions.
 *
 * A run-time comparison to check that Swfdec's version is greater than
 * or equal to version X.Y.Z could be performed as follows:
 *
 * <informalexample><programlisting>
 * if (swfdec_version() >= CAIRO_VERSION_ENCODE(X,Y,Z)) {...}
 * </programlisting></informalexample>
 *
 * See also swfdec_version_string() as well as the compile-time
 * equivalents %SWFDEC_VERSION and %SWFDEC_VERSION_STRING.
 *
 * Return value: the encoded version.
 **/
guint
swfdec_version (void)
{
  return SWFDEC_VERSION;
}

/**
 * swfdec_version_string:
 *
 * Returns the version of the Swfdec library as a human-readable string
 * of the form "X.Y.Z".
 *
 * See also swfdec_version() as well as the compile-time equivalents
 * %SWFDEC_VERSION_STRING and %SWFDEC_VERSION.
 *
 * Return value: a string containing the version.
 **/
const char *
swfdec_version_string (void)
{
  return SWFDEC_VERSION_STRING;
}
