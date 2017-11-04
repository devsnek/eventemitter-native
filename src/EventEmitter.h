#ifndef EVENTEMITTER_NATIVE_H
#define EVENTEMITTER_NATIVE_H

#include <map>
#include <napi.h>

class EventEmitter : public Napi::ObjectWrap<EventEmitter> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  EventEmitter(const Napi::CallbackInfo& info);
  static int defaultMaxListeners;

 private:
  static Napi::FunctionReference constructor;

  Napi::Value On(const Napi::CallbackInfo& info);
  Napi::Value Once(const Napi::CallbackInfo& info);
  Napi::Value Emit(const Napi::CallbackInfo& info);
  Napi::Value ListenerCount(const Napi::CallbackInfo& info);
  Napi::Value Listeners(const Napi::CallbackInfo& info);
  Napi::Value SetMaxListeners(const Napi::CallbackInfo& info);

  void CheckListenerCount(Napi::Env env, Napi::String name);

  bool HasOnceEvent(Napi::String);
  bool HasEveryEvent(Napi::String);

  typedef std::vector<Napi::FunctionReference> Handlers;
  typedef std::map<std::string, Handlers> HandlersMap;

  HandlersMap every;
  HandlersMap once;

  bool warned = false;
  int maxListeners;
};

#endif
