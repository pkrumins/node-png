var fs  = require('fs');
var Png = require('../png').Png;

var rgba = fs.readFileSync('./bw_551x779.dat');

var png = new Png(rgba, 551, 779, 'bw');
var png_image = png.encodeSync();

fs.writeFileSync('./png.png', png_image.toString('binary'), 'binary');

