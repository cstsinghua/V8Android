log('a starting');
exports.done = false;
const b = require('b');
log('in a, b.done = %s', b.done);
exports.done = true;
log('a done');