node-png
--------

This is a node.js module, writen in C++, that uses libpng to produce a PNG
image (in memory) from RGB or RGBA buffers.

The module exports three objects: `Png`, `FixedPngStack` and `DynamicPngStack`.

The `Png` object is for creating PNG images from an RGB, RGBA, or Grayscale buffer.
The `FixedPngStack` is for joining a number of PNGs together (stacking them
together) on a transparent blackground.
The `DynamicPngStack` is for joining a number of PNGs together in the most
space efficient way (so that the canvas border matches the leftmost upper corner
of some PNG and the rightmost bottom corner of some PNG).


Png
---

The `Png` object takes 5 arguments in its constructor:

``` javascript
var png = new Png(buffer, width, height, buffer_type, bits_per_pixel);
```

The first argument, `buffer`, is a node.js `Buffer` filled with RGB(A) values.
The second argument is integer width of the image.
The third argument is integer height of the image.
The fourth argument is 'rgb', 'bgr', 'rgba', 'bgra', or 'gray'. Defaults to 'rgb'.
The fifth argument is valid only when buffer_type='gray'. Valid arguments are 8 (default) and 16.

The constructed `png` object has the `encode` method that's asynchronous in nature.
You give it a callback and it will call your function with a node.js Buffer object
containing the encoded PNG data when it's done:

``` javascript
png.encode(function (png_image) {
    // ...
});
```

The constructed `png` object also has `encodeSync` method that does the encoding
synchronously and returns Buffer with PNG image data:

``` javascript
var png_image = png.encode();
```

You can either send the png_image to the browser, or write to a file, or
do something else with it. See `examples/` directory for some examples.


FixedPngStack
-------------

The `FixedPngStack` object takes 3 arguments in its constructor:

``` javascript
var fixed_png = new FixedPngStack(width, height, buffer_type);
```

The first argument is integer width of the canvas image.
The second argument is integer height of the canvas image.
The third argument is 'rgb', 'bgr', 'rgba or 'bgra'. Defaults to 'rgb'.

Now you can use the `push` method of `fixed_png` object to push buffers
to the canvas. The `push` method takes 5 arguments:

``` javascript
fixed_png.push(buffer, x, y, w, h);
```

It pushes an RGB(A) image in `buffer` of width `w` and height `h` to the canvas
position (x, y). You can push as many buffers to canvas as you want. After
that you should call `encode` method or `encodeSync` method that will join all
the pushed RGB(A) buffers together and return a single PNG.

All the regions that did not get covered will be transparent.


DynamicPngStack
---------------

The `DynamicPngStack` object doesn't take any dimension arguments because its
width and height is dynamically computed. To create it, do:

``` javascript
var dynamic_png = new DynamicPngStack(buffer_type);
```

The `buffer_type` again is 'rgb', 'bgr', 'rgba' or 'bgra', depending on what type
of buffers you're gonna push to `dynamic_png`.

It provides four methods - `push`, `encode`, `encodeSync`, and `dimensions`. The
`push` and `encode` methods are the same as in `FixedPngStack`. You `push` each
of the RGB(A) buffers to the stack and after that you call `encode` or
`encodeSync`.

The `encode` asynchronous method receives one more argument than others - it
receives the dimensions object with x, y, width and height of the dynamic PNG.
See the next paragraph for what the dimensions are.

The `dimensions` method is more interesting. It must be called only after
`encode` as its values are calculated upon encoding the image. It returns an
object with `width`, `height`, `x` and `y` properties. The `width` and
`height` properties show the width and the height of the final image. The `x`
and `y` propreties show the position of the leftmost upper PNG.

Here is an example that illustrates it. Suppose you wish to join two PNGs
together. One with width 100x40 at position (5, 10) and the other with
width 20x20 at position (2, 210). First you create the DynamicPngStack
object:

``` javascript
var dynamic_png = new DynamicPngStack();
```

Next you push the RGB(A) buffers of the two PNGs to it:

``` javascript
dynamic_png.push(png1_buf, 5, 10, 100, 40);
dynamic_png.push(png2_buf, 2, 210, 20, 20);
```

Now you can call `encode` to produce the final PNG:

``` javascript
var png = dynamic_png.encodeSync();
```

Now let's see what the dimensions are,

``` javascript
var dims = dynamic_png.dimensions();
```

Same asynchronously:

``` javascript
dynamic_png.encode(function (png, dims) {
    // png is the PNG image (in a node.js Buffer)
    // dims are its dimensions
});
```

The x position `dims.x` is 2 because the 2nd png is closer to the left.
The y position `dims.y` is 10 because the 1st png is closer to the top.
The width `dims.width` is 103 because the first png stretches from x=5 to
x=105, but the 2nd png starts only at x=2, so the first two pixels are not
necessary and the width is 105-2=103.
The height `dims.height` is 220 because the 2nd png is located at 210 and
its height is 20, so it stretches to position 230, but the first png starts
at 10, so the upper 10 pixels are not necessary and height becomes 230-10= 220.


How to compile?
---------------

To get the node-png module compiled, you need to have libpng and node.js
installed. Then just run:

``` bash
    node-gyp configure build
```

to build node-png module. It will be called `png.node`. To use it, make sure
it's in NODE_PATH.

See also http://github.com/pkrumins/node-jpeg module that produces JPEG images.
And also http://github.com/pkrumins/node-gif for producing GIF images.

If you wish to stream PNGs over a websocket or xhr-multipart, you'll have to
base64 encode it. Use my http://github.com/pkrumins/node-base64 module to do
that.

