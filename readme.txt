This is a node.js module, writen in C++, that uses libpng to produce a PNG
image (in memory) from a buffer of RGBA values.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

The module exports `Png` object that takes 3 arguments in its constructor:

    var png = new Png(buffer, width, height);

The first argument, `buffer`, is a nodee.js `Buffer` filled with RGBA values.
The second argument is integer width of the image.
The third argument is integer height of the image.

The constructed `png` object then has an `encode` method that when called,
converts the RGBA values in buffer into a PNG image and returns as a binary
string.

See `examples/` directory for examples.

To get it compiled, you need to have libpng and node installed. Then just run

    node-waf configure build

to build Png module. It will be called `png.node`. Make sure it's in NODE_PATH
to use it.

See also http://github.com/pkrumins/node-jpeg module that produces JPEG images.


------------------------------------------------------------------------------

Have fun!


Sincerely,
Peteris Krumins
http://www.catonmat.net

