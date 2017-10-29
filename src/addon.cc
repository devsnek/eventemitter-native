#include <napi.h>
#include "EventEmitter.h"

Napi::Object CreateObject(const Napi::CallbackInfo& info) {
  return EventEmitter::NewInstance(info.Env());
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  Napi::Object new_exports = Napi::Function::New(env, CreateObject, "EventEmitter");
  return EventEmitter::Init(env, new_exports);
}

NODE_API_MODULE(addon, InitAll)
