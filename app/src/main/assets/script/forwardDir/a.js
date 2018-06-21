sun.log('a starting');
exports.done = false;
const b = require('../../b');
sun.log('in a, b.done = %s', b.done);
exports.done = true;
sun.log('a done');