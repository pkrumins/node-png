This is a node.js module, writen in C++, that uses libpng to produce a PNG
image from a buffer of RGBA values.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

The module exports `Png` object that takes 3 arguments in its constructor:
    1. A node.js `Buffer` filled with RGBA values.
    2. Width.
    3. Height.

The constructed `Png` object then has an `encode` method that when called,
starts emitting `data` events with `chunk` and `length` paramters. When it's
done encoding, it emits `end` event.

See `tests/` directory for examples.

------------------------------------------------------------------------------


Sincerely,
Peteris Krumins
http://www.catonmat.net

