var fs  = require('fs');
var sys = require('sys');
var Png = require('../png').Png;

var rgba = fs.readFileSync('./rgba-terminal.dat');

var png = new Png(rgba, 720, 400, 'rgba');
var png_image = png.encodeSync();

fs.writeFileSync('./png.png', png_image.toString('binary'), 'binary');

