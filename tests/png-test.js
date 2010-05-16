var fs  = require('fs');
var sys = require('sys');
var Png = require('../png').Png;
var Buffer = require('buffer').Buffer;

// the rgba-terminal.dat file is 1152000 bytes long.
var rgba = new Buffer(1152000);
rgba.write(fs.readFileSync('./rgba-terminal.dat', 'binary'), 'binary');

var png = new Png(rgba, 720, 400);

var fd = fs.openSync('./png.png', 'w+', 0660);
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

