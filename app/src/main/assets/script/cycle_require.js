log('main starting');
log('require :'+ require);
log('module :'+ module);
log('exports :'+ exports);
const a = require('a');
const b = require('b');
log('in main, a.done = %s, b.done = %s', a.done, b.done);