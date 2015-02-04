# Please Note

This code is almost old enough to drive.

A few people still use it, though(!), and I still get patches from time to
time. I guess I should finally at least put it on the Githubs and make it easy
for those people to help each other out.

Here's the text of the original README:

```
gkrellkam.so -- image watcher plugin for GKrellM
Copyright (C) 2001-2002 paul cannon

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, USA.

To contact the author use:
<pik@debian.org>

------------------------------------------------------------------
See the file COPYING for license info (you've probably seen it
before..).

See the file INSTALL for the simplistic install instructions.
------------------------------------------------------------------

I wrote GKrellKam because there's a webcam pointing at the building where I
work, and I thought it would be nice to see what the weather was like outside
without taking up much space on the desktop. GKrellWeather is great, but this
is a little more.. well, graphic.

The idea is that you can have a periodically updated image of whatever you
want, albeit squeezed into a little gkrellm panel. Each panel can watch a
single image, or cycle through a list of images, or run a script and use the
output of the script as the filename of an image. GKrellKam even knows how to
get images out on the internet- this is what allows you to watch webcams.

GKrellKam allows lists to be compiled of image sources for the different
panels. Panels with lists cycle through the different sources. You can
specify local files as lists, or give a URL to an online one. Online lists
have the same syntax. All lists should end in ".list" or "-list". For more
information, see the plugin's Info text, and the included gkrellkam-list(5)
manpage.

If you don't want to mess around with all that, just put the address of a
webcam in the 'Image Source' configuration box, set the number of seconds
for each update, and GKrellKam will do the rest.

Thanks to Benjamin Johnson (benj@visi.com) for some of the image scaling
code.

Many, many thanks to Bill Wilson for doing the port to gkrellm2 (version
2.0.0).
```
