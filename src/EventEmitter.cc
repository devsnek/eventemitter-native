#include <napi.h>
#include <uv.h>
#include "EventEmitter.h"

Napi::FunctionReference EventEmitter::constructor;
int EventEmitter::defaultMaxListeners;

Napi::Object EventEmitter::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  EventEmitter::defaultMaxListeners = Napi::Number::New(env, 10);

  Napi::Function func = DefineClass(env, "EventEmitter", {
    InstanceMethod("on", &EventEmitter::On),
    InstanceMethod("once", &EventEmitter::Once),
    InstanceMethod("emit", &EventEmitter::Emit),
    InstanceMethod("listenerCount", &EventEmitter::ListenerCount),
    InstanceMethod("listeners", &EventEmitter::Listeners),
    InstanceMethod("setMaxListeners", &EventEmitter::SetMaxListeners),
    StaticValue("usingDomains", Napi::Boolean::New(env, false), napi_enumerable),
    // FIXME: weird type issues with getter and setter
    /*StaticAccessor("defaultMaxListeners",
        &EventEmitter::GetDefaultMaxListeners, &EventEmitter::SetDefaultMaxListeners, napi_enumerable),*/
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("EventEmitter", func);

  return exports;
}

EventEmitter::EventEmitter(const Napi::CallbackInfo& info) : Napi::ObjectWrap<EventEmitter>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  this->maxListeners = EventEmitter::defaultMaxListeners;
};

bool EventEmitter::HasEveryEvent(Napi::String name) {
  return this->every.find(name) != this->every.end();
}

bool EventEmitter::HasOnceEvent(Napi::String name) {
  return this->once.find(name) != this->once.end();
}

Napi::Value EventEmitter::SetDefaultMaxListeners(const Napi::CallbackInfo& info, const Napi::Value& value) {
  EventEmitter::defaultMaxListeners = value.As<Napi::Number>();
  return value;
}

Napi::Value EventEmitter::GetDefaultMaxListeners(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), EventEmitter::defaultMaxListeners);
}

void EventEmitter::CheckListenerCount(Napi::Env env, Napi::String name) {
  size_t count = 0;
  if (this->HasEveryEvent(name))
    count += this->every[name].size();
  if (this->HasOnceEvent(name))
    count += this->once[name].size();

  if ((int)count <= this->maxListeners) return;

  Napi::Object global = env.Global().As<Napi::Object>();
  Napi::Object process = global.Get("process").As<Napi::Object>();
  Napi::Function emitWarning = process.Get("emitWarning").As<Napi::Function>();

  // TODO: better message
  Napi::Error err = Napi::Error::New(env, "Possible EventEmitter memory leak detected.");
  err.Set("name", Napi::String::New(env, "MaxListenersExceededWarning"));
  // err.Set("emitter", this);
  err.Set("type", name);
  err.Set("count", Napi::Number::New(env, (int)count));

  // emitWarning({ err }); convert Napi::Error to Napi::Value
}

Napi::Value EventEmitter::On(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  if (!this->HasEveryEvent(name))
    this->every[name] = {};

  this->every[name].push_back(handler);

  this->CheckListenerCount(env, name);

  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::Once(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  if (!this->HasOnceEvent(name))
    this->once[name] = {};

  this->once[name].push_back(handler);

  this->CheckListenerCount(env, name);

  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::Emit(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();

  bool hasOnce = this->HasOnceEvent(name);
  bool hasEvery = this->HasEveryEvent(name);
  if (!hasOnce && !hasEvery)
    return Napi::Boolean::New(env, false);

  std::vector<napi_value> callArgs;
  for (size_t i = 1; i < info.Length(); i++)
    callArgs[i - 1] = info[i];

  if (hasEvery) {
    for (Napi::Function const& handler: this->every[name])
      handler.Call(callArgs);
  }

  if (hasOnce) {
    for (Napi::Function const& handler: this->once[name])
      handler.Call(callArgs);

    this->once.erase(name);
  }


  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::ListenerCount(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();

  bool hasOnce = this->HasOnceEvent(name);
  bool hasEvery = this->HasEveryEvent(name);

  if (!hasOnce && !hasEvery)
    return Napi::Number::New(env, 0);

  if (hasOnce && hasEvery)
    return Napi::Number::New(env, this->every[name].size() + this->once[name].size());
  else if (hasOnce)
    return Napi::Number::New(env, this->once[name].size());
  else if (hasEvery)
    return Napi::Number::New(env, this->every[name].size());
}

Napi::Value EventEmitter::Listeners(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();

  std::vector<Napi::Function> once = this->once[name];
  std::vector<Napi::Function> every = this->every[name];
  bool hasOnce = this->HasOnceEvent(name);
  bool hasEvery = this->HasEveryEvent(name);

  if (!hasOnce && !hasEvery)
    return Napi::Array::New(env, 0);

  Napi::Array array = Napi::Array::New(env, once.size());

  if (hasOnce) {
    for (size_t i = 0; i < once.size(); i++)
      array[i] = every[i];
  }

  if (hasEvery) {
    size_t offset = hasOnce ? once.size() : 0;
    for (size_t i = 0; i < every.size(); i++)
      array[i + offset] = every[i];
  }

  // FIXME: array always filled with 5s
  return array;
}

Napi::Value EventEmitter::SetMaxListeners(const Napi::CallbackInfo& info) {
  Napi::Number max = info[0].As<Napi::Number>();
  this->maxListeners = max;
  return max;
}
