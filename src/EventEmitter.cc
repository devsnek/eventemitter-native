#include <napi.h>
#include <uv.h>
#include "EventEmitter.h"

Napi::FunctionReference EventEmitter::constructor;
int EventEmitter::defaultMaxListeners;

void SetDefaultMaxListeners(const Napi::CallbackInfo& info, const Napi::Value& value) {
  EventEmitter::defaultMaxListeners = value.As<Napi::Number>();
}

Napi::Value GetDefaultMaxListeners(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), EventEmitter::defaultMaxListeners);
}

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
    StaticAccessor("defaultMaxListeners", GetDefaultMaxListeners, SetDefaultMaxListeners, napi_enumerable),
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

void EventEmitter::CheckListenerCount(Napi::Env env, Napi::String name) {
  if (this->warned) return;

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
  char message[256];
  sprintf(message,
      "Possible EventEmitter memory leak detected. %zu %s listeners added. "
      "Use emitter.setMaxListeners() to increase limit",
      count, name.Utf8Value().c_str());
  Napi::Error err = Napi::Error::New(env, message);
  err.Set("name", Napi::String::New(env, "MaxListenersExceededWarning"));
  err.Set("emitter", *this);
  err.Set("type", name);
  err.Set("count", Napi::Number::New(env, (int)count));
  this->warned = true;
  emitWarning({ err.Value() });
}

Napi::Value EventEmitter::On(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  this->InternalEmit(Napi::String::New(env, "newListener"), { name, handler });

  this->every[name].push_back(Napi::Reference<Napi::Function>::New(handler, 1));

  this->CheckListenerCount(env, name);

  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::Once(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::String name = info[0].As<Napi::String>();
  Napi::Function handler = info[1].As<Napi::Function>();

  this->InternalEmit(Napi::String::New(env, "newListener"), { name, handler });

  this->once[name].push_back(Napi::Reference<Napi::Function>::New(handler, 1));

  this->CheckListenerCount(env, name);

  return Napi::Boolean::New(env, true);
}

Napi::Value EventEmitter::Emit(const Napi::CallbackInfo& info) {
  Napi::String name = info[0].As<Napi::String>();

  std::vector<napi_value> args;
  args.resize(info.Length() - 1);
  for (size_t i = 1; i < info.Length(); i++)
    args[i - 1] = info[i];

  return Napi::Boolean::New(info.Env(), this->InternalEmit(name, args));
}

bool EventEmitter::InternalEmit(Napi::String name, std::vector<napi_value> args) {
  bool hasOnce = this->HasOnceEvent(name);
  bool hasEvery = this->HasEveryEvent(name);
  if (!hasOnce && !hasEvery)
    return false;

  if (hasEvery) {
    std::vector<Napi::Function> list;
    for (Napi::FunctionReference &ref : this->every[name])
      list.push_back(ref.Value());
    for (Napi::Function &func : list)
      func.Call(args);
  }

  if (hasOnce) {
    std::vector<Napi::Function> list;
    for (Napi::FunctionReference &ref: this->every[name])
      list.push_back(ref.Value());

    for (Napi::Function &func: list)
      func.Call(args);

    this->once.erase(name);
  }

  return true;
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
  return Napi::Array::New(env, 0);
}

Napi::Value EventEmitter::SetMaxListeners(const Napi::CallbackInfo& info) {
  Napi::Number max = info[0].As<Napi::Number>();
  this->maxListeners = max;
  return max;
}
