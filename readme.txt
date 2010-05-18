This is a node.js module, writen in C++, that uses libpng to produce a PNG
image from a buffer of RGBA values.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

The module exports `Png` object that takes 3 arguments in its constructor:

    var png = new Png(buffer, width, height);

The first argument, `buffer`, is a nodee.js `Buffer` filled with RGBA values.
The second argument is integer width of the image.
The third argument is integer height of the image.

The constructed `png` object then has an `encode` method that when called,
starts emitting `data` events with `chunk` and `length` paramters. When it's
done encoding, it emits `end` event.

See `examples/` directory for examples.

To get it compiled, you need to have libpng and node installed. Then just run

    node-waf configure build

to build Png module. It will build `png.node` module.

See also http://github.com/pkrumins/node-jpeg to produce JPEG images.


------------------------------------------------------------------------------

Have fun!


Sincerely,
Peteris Krumins
http://www.catonmat.net

