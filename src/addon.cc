#include <napi.h>
#include "EventEmitter.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  EventEmitter::Init(env, exports);
  return exports;
}

NODE_API_MODULE(addon, InitAll)
