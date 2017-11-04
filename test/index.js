const EventEmitter = require('../');

const e = new EventEmitter();

e.on('test1', console.log);
e.once('test1', console.log);

for (let i = 0; i < 10; i++) e.on('test1', console.log);

console.log(e.listenerCount('test1'));

e.emit('test1', 'works');
