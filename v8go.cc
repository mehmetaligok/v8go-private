#include "v8go.h"

#include "v8.h"
#include "libplatform/libplatform.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <stdio.h>

using namespace v8;

auto default_platform = platform::NewDefaultPlatform();
auto default_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();

typedef struct {
  Persistent<Context> ptr;
  Isolate* iso;
} m_ctx;

typedef struct {
  Persistent<Value> ptr;
  m_ctx* ctx_ptr;
} m_value;


extern "C"
{

/********** Isolate **********/

void Init() {
    V8::InitializePlatform(default_platform.get());
    V8::Initialize();
    return;
}

IsolatePtr NewIsolate() {
    Isolate::CreateParams params;
    params.array_buffer_allocator = default_allocator;
    return static_cast<IsolatePtr>(Isolate::New(params));
}

void IsolateDispose(IsolatePtr ptr) {
    if (ptr == nullptr) {
        return;
    }
    Isolate* iso = static_cast<Isolate*>(ptr);
    iso->Dispose();
}

void TerminateExecution(IsolatePtr ptr) {
    Isolate* iso = static_cast<Isolate*>(ptr);
    iso->TerminateExecution();
}

/********** Context **********/

ContextPtr NewContext(IsolatePtr ptr) {
    Isolate* iso = static_cast<Isolate*>(ptr);
    Locker locker(iso);
    Isolate::Scope isolate_scope(iso);
    HandleScope handle_scope(iso);
  
    iso->SetCaptureStackTraceForUncaughtExceptions(true);
    
    m_ctx* ctx = new m_ctx;
    ctx->ptr.Reset(iso, Context::New(iso));
    ctx->iso = iso;
    return static_cast<ContextPtr>(ctx);
}

ValuePtr RunScript(ContextPtr ctx_ptr, const char* source, const char* origin) {
    m_ctx* ctx = static_cast<m_ctx*>(ctx_ptr);
    Isolate* iso = ctx->iso;
    Locker locker(iso);
    Isolate::Scope isolate_scope(iso);
    HandleScope handle_scope(iso);
    TryCatch try_catch(iso);

    Local<Context> local_ctx = ctx->ptr.Get(iso);
    Context::Scope context_scope(local_ctx);

    Local<String> src = String::NewFromUtf8(iso, source, NewStringType::kNormal).ToLocalChecked();
    Local<String> ogn = String::NewFromUtf8(iso, origin, NewStringType::kNormal).ToLocalChecked();

    ScriptOrigin script_origin(ogn);
    Local<Script> script = Script::Compile(local_ctx, src, &script_origin).ToLocalChecked();
    
    // TODO check to make sure script is not empty
    Local<v8::Value> result = script->Run(local_ctx).ToLocalChecked();
    
    // TODO deal with any errors/exceptions from result
    m_value* val = new m_value;
    val->ctx_ptr = ctx;
    val->ptr.Reset(iso, Persistent<Value>(iso, result));

    return static_cast<ValuePtr>(val);
}

/********** Value **********/

const char* ValueToString(ValuePtr val_ptr) {
  m_value* val = static_cast<m_value*>(val_ptr);
  m_ctx* ctx = val->ctx_ptr;
  Isolate* iso = ctx->iso;

  Locker locker(iso);
  Isolate::Scope isolate_scope(iso);
  HandleScope handle_scope(iso);
  Context::Scope context_scope(ctx->ptr.Get(iso));

  Local<Value> value = val->ptr.Get(iso);
  String::Utf8Value utf8(iso, value);
  
  char* data = static_cast<char*>(malloc(utf8.length()));
  //memcpy(data, *utf8, utf8.length());
  sprintf(data, "%s", *utf8);
  return data;
}


/********** Version **********/
  
const char* Version() {
    return V8::GetVersion();
}

}


int _main(int argc, char* argv[]) {
    Init();
    auto i = NewIsolate();
    auto c = NewContext(i);
    RunScript(c, "18 + 17", "");
    return 0;
}
