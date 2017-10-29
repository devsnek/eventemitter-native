const EventEmitter = require('../');

const e = new EventEmitter();

e.on('test1', console.log);

e.emit('test1', 'works');
