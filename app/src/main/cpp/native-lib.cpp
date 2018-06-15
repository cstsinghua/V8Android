#include <jni.h>
#include <string>
#include <android/log.h>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include "include/v8-version-string.h"
#include "log.h"
#include "util.h"

/**
 * Must stub in case external snapshot files are used.
 */
namespace v8::internal {
    void ReadNatives() {}

    void DisposeNatives() {}

    void SetNativesFromFile(v8::StartupData *s) {}

    void SetSnapshotFromFile(v8::StartupData *s) {}
}

using namespace v8;
Isolate *isolate;
Global<Context> context_;
Global<Function> function_;
Isolate::CreateParams create_params;
Gamer *gamer;

const char *executeJSFromString(const char *ch);

void initV8();

// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
    return *value ? *value : "<string conversion failed>";
}

void js_log(const FunctionCallbackInfo<Value> &args) {
    int len = args.Length();
    if (len < 1) return;
    std::string printstr("");
    for (int i = 0; i < len; ++i) {
        String::Utf8Value utf8(isolate, args[i]);
        if (i == 0) {
            printstr.append(ToCString(utf8));
        } else {
            int location = printstr.find("%s");
            if (location >= 0) {
                printstr = printstr.replace(location, 2, ToCString(utf8));
            }
        }

    }
    LOGV("js log:%s", printstr.c_str());
}

void js_require(const FunctionCallbackInfo<Value> &args) {
    String::Utf8Value utf8(isolate, args[0]);
    std::string str("script/");
    std::string str1(ToCString(utf8));
    std::string str2(".js");
    str.append(str1).append(str2);
    Handle<Value> module = ExecuteJSScript2(isolate, str.c_str(), true, true);
    if (module->IsStringObject()) {
        LOGI("return value of js module");
    }
    args.GetReturnValue().Set(module);


}

void js_readFile(const FunctionCallbackInfo<Value> &args) {
    // LOGI("js log:%d",args.Length());
    String::Utf8Value utf8(isolate, args[0]);
    const char *cstr = ToCString(utf8);
    const char *source = openScriptFile(cstr);
    Local<String> js_src = String::NewFromUtf8(isolate, source,
                                               NewStringType::kNormal).ToLocalChecked();
    args.GetReturnValue().Set(js_src);
}

void js_compileJSCode(const FunctionCallbackInfo<Value> &args) {
    // LOGI("js log:%d",args.Length());
    String::Utf8Value utf8(isolate, args[0]);
    const char *cstr = ToCString(utf8);
    Local<String> js_src = String::NewFromUtf8(isolate, cstr,
                                               NewStringType::kNormal).ToLocalChecked();
    Handle<Value> func = ExecuteJSScript3(isolate, js_src, true, true);
    args.GetReturnValue().Set(func);
}

void initV8() {// Initialize V8.
//    v8::V8::InitializeICUDefaultLocation(nullptr);
//    v8::V8::InitializeExternalStartupData(nullptr);
    LOGI("init v8");
    Platform *platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();
    // Create a new Isolate and make it the current one.
    // Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
            ArrayBuffer::Allocator::NewDefaultAllocator();
    //v8::Isolate* isolate = v8::Isolate::New(create_params);
    isolate = Isolate::New(create_params);
    Isolate::Scope isolate_scope(isolate);
    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);

    Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
    //expose C++ log function to JS global context
    global->Set(isolate, "log", FunctionTemplate::New(isolate, js_log));
    //expose require function to JS global context
    // global->Set(isolate, "require", FunctionTemplate::New(isolate, js_require));

    //expose readFile function to JS global context
    global->Set(isolate, "readFile", FunctionTemplate::New(isolate, js_readFile));
    //expose compileJSCode function to JS global context
    global->Set(isolate, "compileJSCode", FunctionTemplate::New(isolate, js_compileJSCode));

    // global->Set(isolate,"globalGamer",WrapGamerObject(isolate,gamer)); //this way is error

    //expose C++ globalGamer object to JS global context
    global->SetAccessor(
            String::NewFromUtf8(isolate, "globalGamer", NewStringType::kInternalized)
                    .ToLocalChecked(),
            GetGamer);

    //create context containing global obj above
    v8::Local<v8::Context> context = Context::New(isolate, NULL, global);
//    context->Global()->Set(String::NewFromUtf8(isolate, "globalGamer", NewStringType::kNormal)
//                                   .ToLocalChecked(),WrapGamerObject(isolate,gamer));
    context_.Reset(isolate, context);
    // Enter the new context so all the following operations take place
    // within it.
    Context::Scope context_scope(context);
    // Create a string containing the JavaScript source code.
    // bool result = ExecuteJSScript(isolate, "script/main.js", true, true);
    bool result = ExecuteJSScript(isolate, "script/module.js", true, true);
    LOGI("JS Script Execute Result :%d", result);

//    Local<String> process_name =
//            String::NewFromUtf8(isolate, "Game", NewStringType::kNormal)
//                    .ToLocalChecked();
    Local<String> process_name =
            String::NewFromUtf8(isolate, "runMain", NewStringType::kNormal)
                    .ToLocalChecked();
    Local<Value> process_val;
    // If there is no Process function, or if it is not a function,
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
        // LOGI("Game is not a function\n");
        LOGI("runMain is not a function\n");
    }
    // It is a function; cast it to a Function
    Local<Function> process_fun = Local<Function>::Cast(process_val);
    function_.Reset(isolate, process_fun);
}


const char *executeJSFromString(const char *ch) {

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate);

    // Create a new context.
    Local<Context> context = Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    Local<String> source =
            String::NewFromUtf8(isolate, ch,
                                NewStringType::kNormal)
                    .ToLocalChecked();

    // Compile the source code.
    Local<Script> script =
            Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    Local<Value> result = script->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(isolate, result);
    LOGV("step v8 string:%s.", *utf8);
    // printf("%s\n", *utf8);
    return ToCString(utf8);
}

//call js function Game(gamer)
void executeJSFunction() {
    // Create a handle scope to keep the temporary object references.
    HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context =
            v8::Local<v8::Context>::New(isolate, context_);
    // Enter this processor's context so all the remaining operations
    // take place there
    Context::Scope context_scope(context);

    // Set up an exception handler before calling the Process function
    TryCatch try_catch(isolate);

//    const int argc = 1;
//    Local<Value> argv[argc] = {WrapGamerObject(isolate, gamer)};
    const int argc = 0;
    Local<Value> argv[argc] = {};
    v8::Local<v8::Function> process =
            v8::Local<v8::Function>::New(isolate, function_);
    Local<Value> result;
    if (!process->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(isolate, try_catch.Exception());
        LOGI("call js function error:%s", *error);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_boyaa_v8wrapper_MainActivity_nativeInit(JNIEnv *env, jobject instance,
                                                 jobject assetManager) {

    LOGI("setEnvAndAssetManager");
    setEnvAndAssetManager(env, assetManager);
    initV8();

}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_boyaa_v8wrapper_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    gamer = new Gamer(18, "Stephen");
    const char *result = "hello v8";

    try {
        //execute js statement
        const char *statement = "\"Hello,World!\"";
        result = executeJSFromString(statement);
    } catch (std::exception &e) {
        LOGV("%s", e.what());
    }

    LOGV("v8 version:%s", V8_VERSION_STRING);
    LOGV("js string:%s", result);
    return env->NewStringUTF(result);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_boyaa_v8wrapper_MainActivity_executeJSScript(JNIEnv *env, jobject instance) {

    //execute js function
    executeJSFunction();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_boyaa_v8wrapper_MainActivity_nativeDestroy(JNIEnv *env, jobject instance) {
    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
}