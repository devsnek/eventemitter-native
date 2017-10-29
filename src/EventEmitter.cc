#include <napi.h>
#include <uv.h>
#include "EventEmitter.h"

Napi::FunctionReference EventEmitter::constructor;

Napi::Object EventEmitter::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "EventEmitter", {
    InstanceMethod("on", &EventEmitter::On),
    InstanceMethod("once", &EventEmitter::Once),
    InstanceMethod("emit", &EventEmitter::Emit),
    InstanceMethod("listenerCount", &EventEmitter::ListenerCount),
    InstanceMethod("listeners", &EventEmitter::Listeners)
  });

  // require('events').EventEmitter :(
  napi_set_named_property(env, func, "EventEmitter", func);

  // this is probably important
  napi_set_named_property(env, func, "usingDomains", Napi::Boolean::New(env, false));

  // TODO: make getter/setter
  napi_set_named_property(env, func, "defaultMaxListeners", Napi::Number::New(env, EventEmitter::defaultMaxListeners));

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("EventEmitter", func);

  return exports;
}

EventEmitter::EventEmitter(const Napi::CallbackInfo& info) : Napi::ObjectWrap<EventEmitter>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
};

Napi::Object EventEmitter::NewInstance(Napi::Env env) {
  Napi::EscapableHandleScope scope(env);
  Napi::Object obj = constructor.New({});
  return scope.Escape(napi_value(obj)).ToObject();
}

bool EventEmitter::HasEveryEvent(Napi::String name) {
  return this->every.find(name) != this->every.end();
}

bool EventEmitter::HasOnceEvent(Napi::String name) {
  return this->once.find(name) != this->once.end();
}

Napi::Value EventEmitter::On(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  if (!this->HasEveryEvent(name))
    this->every[name] = {};

  this->every[name].push_back(handler);

  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::Once(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  if (!this->HasOnceEvent(name))
    this->once[name] = {};

  this->once[name].push_back(handler);

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

    this->once[name].clear();
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

  // FIX: array always filled with 5s
  return array;
}
