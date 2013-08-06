/* Demonstrate a 16-bit grayscale image */

var fs  = require('fs');
var sys = require('sys');
var Png = require('../png').Png;
var Buffer = require('buffer').Buffer;

var WIDTH = 300, HEIGHT = 300;

var img_buffer = new Buffer(WIDTH*HEIGHT*2);

for (var i=0; i<HEIGHT; i++) {
  var val = 65535 * (i/HEIGHT) // make a vertical gradient
    for (var j=0; j<WIDTH; j++) {
        img_buffer[i*WIDTH*2 + j*2 + 0] = val >> 8;
        img_buffer[i*WIDTH*2 + j*2 + 1] = val & 0xFF;
    }
}

var png = new Png(img_buffer, WIDTH, HEIGHT, 'gray', 16);

fs.writeFileSync('./png-gray-gradient-16.png', png.encodeSync().toString('binary'), 'binary');

