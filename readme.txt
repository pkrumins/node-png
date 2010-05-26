This is a node.js module, writen in C++, that uses libpng to produce a PNG
image (in memory) from a buffer of RGBA values.

It was written by Peteris Krumins (peter@catonmat.net).
His blog is at http://www.catonmat.net  --  good coders code, great reuse.

------------------------------------------------------------------------------

The module exports two objects - `Png` and `FixedPngStack`. The `Png` object
is for creating PNG images from an RGBA buffer, and `FixedPngStack` object is
for joining a number of PNGs together (stacking them together) on a
transparent blackground.


The `Png` object takes 3 arguments in its constructor:

    var png = new Png(buffer, width, height);

The first argument, `buffer`, is a node.js `Buffer` filled with RGBA values.
The second argument is integer width of the image.
The third argument is integer height of the image.

The constructed `png` object then has an `encode` method that when called,
converts the RGBA values in buffer into a PNG image and returns as a binary
string:

    var png_image = png.encode();

You can now either send the png_image to the browser, or write to a file, or
do something else with it. See `examples/` directory for more examples.


The `FixedPngStack` object takes two arguments in its constructor:

    var fixed_png = new FixedPngStack(width, height);

The first argument is integer width of the canvas image.
The second argument is integer height of the canvas iamge.

Now you can use the `push` method of `fixed_png` object to push RGBA buffers
to the canvas. The `push` method takes 5 arguments:

    fixed_png.push(buffer, x, y, w, h);

It pushes an RGBA image in `buffer` of width `w` and height `h` to the canvas
position (x, y). You can push as many buffers to canvas as you want. After
that you should call `encode` method that will join all the pushed RGBA
buffers together and return a single PNG.

All the regions that did not get covered will be transparent.


To get the node-png module compiled, you need to have libpng and node.js
installed. Then just run:

    node-waf configure build

to build node-png module. It will be called `png.node`. To use it, make sure
it's in NODE_PATH.

See also http://github.com/pkrumins/node-jpeg module that produces JPEG images.

If you wish to stream PNGs over a websocket or xhr-multipart, you'll have to
base64 encode it. Use my http://github.com/pkrumins/node-base64 module to do
that.


------------------------------------------------------------------------------

Have fun producing PNGs!


Sincerely,
Peteris Krumins
http://www.catonmat.net

