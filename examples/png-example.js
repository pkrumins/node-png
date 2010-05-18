var fs  = require('fs');
var sys = require('sys');
var Png = require('../png').Png;
var Buffer = require('buffer').Buffer;

// the rgba-terminal.dat file is 1152000 bytes long.
var rgba = new Buffer(1152000);
rgba.write(fs.readFileSync('./rgba-terminal.dat', 'binary'), 'binary');

var png = new Png(rgba, 720, 400);
var png_image = png.encode();

fs.writeFileSync('./png.png', png_image, 'binary');

