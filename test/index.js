const EventEmitter = require('../');

const e = new EventEmitter();

e.on('test1', console.log);
e.once('test1', console.log);

console.log(e.listenerCount('test1'));

e.emit('test1', 'works');
