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

var png = new Png(rgba, 5, 5);

var fd = fs.openSync('./png-5x5.png', 'w+', 0660);
var total = 0, written = 0;
png.addListener('data', function(chunk, length) {
    sys.log('Got a chunk. Size: ' + length);
    written += fs.writeSync(fd, chunk, written, 'binary');
    total += length;
});
png.addListener('end', function() {
    fs.closeSync(fd);
    sys.log('Total: ' + total + ' bytes. Written: ' + written + ' bytes.');
});
png.encode();

