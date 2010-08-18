var fs  = require('fs');
var sys = require('sys');
var Png = require('../png').Png;
var Buffer = require('buffer').Buffer;

var x = "\x00\x00\x00\x00";
var o = "\xff\x00\x00\x00";

var img = x + o + o + o + o +
          o + x + o + o + o +
          o + o + x + o + o +
          o + o + o + x + o +
          o + o + o + o + x;

var rgba = new Buffer(100);
rgba.write(img, 'binary');

var png = new Png(rgba, 5, 5, 'rgba');
var png_image = png.encodeSync();

fs.writeFileSync('./png-5x5.png', png_image.toString('binary'), 'binary');

