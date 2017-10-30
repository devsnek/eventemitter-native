#ifndef EVENTEMITTER_NATIVE_H
#define EVENTEMITTER_NATIVE_H

#include <map>
#include <napi.h>

class EventEmitter : public Napi::ObjectWrap<EventEmitter> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  EventEmitter(const Napi::CallbackInfo& info);

 private:
  static Napi::FunctionReference constructor;

  Napi::Value On(const Napi::CallbackInfo& info);
  Napi::Value Once(const Napi::CallbackInfo& info);
  Napi::Value Emit(const Napi::CallbackInfo& info);
  Napi::Value ListenerCount(const Napi::CallbackInfo& info);
  Napi::Value Listeners(const Napi::CallbackInfo& info);

  bool HasOnceEvent(Napi::String);
  bool HasEveryEvent(Napi::String);

  std::map<Napi::String, std::vector<Napi::Function>> every;
  std::map<Napi::String, std::vector<Napi::Function>> once;

  static const int defaultMaxListeners = 10;
};

#endif
