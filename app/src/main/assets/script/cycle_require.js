sun.log('main starting');
sun.log('require :'+ require);
sun.log('module :'+ module);
sun.log('exports :'+ exports);
const a = require('a');
const b = require('b');
sun.log('in main, a.done = %s, b.done = %s', a.done, b.done);