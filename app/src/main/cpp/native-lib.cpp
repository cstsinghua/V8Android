#include <jni.h>
#include <string>
#include <android/log.h>
#include <v8pp/context.hpp>
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include "include/v8-version-string.h"
#include "log.h"
#include "util.h"
#include "Person.cpp"

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
Isolate *isolate_;
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

void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate_);

    v8::String::Utf8Value value(isolate_, args[0]);
    v8::String::Utf8Value value1(isolate_, args[1]);
    v8::String::Utf8Value value2(isolate_, args[2]);
    v8::String::Utf8Value value3(isolate_, args[3]);
    LOGE(*value,*value1,*value2,*value3);
}

void js_log(const FunctionCallbackInfo<Value> &args) {
    int len = args.Length();
    if (len < 1) return;
    std::string printstr("");
    for (int i = 0; i < len; ++i) {
        String::Utf8Value utf8(isolate_, args[i]);
        if (i == 0) {
            printstr.append(ToCString(utf8));
        } else {
            int location = printstr.find("%s");
            if (location >= 0) {
                printstr = printstr.replace(location, 2, ToCString(utf8));
            }
        }

    }
    LOGD("js log:%s", printstr.c_str());
}

void js_require(const FunctionCallbackInfo<Value> &args) {
    String::Utf8Value utf8(isolate_, args[0]);
    std::string str("script/");
    std::string str1(ToCString(utf8));
    std::string str2(".js");
    str.append(str1).append(str2);
    Handle<Value> module = ExecuteJSScript2(isolate_, str.c_str(), true, true);
    if (module->IsStringObject()) {
        LOGI("return value of js module");
    }
    args.GetReturnValue().Set(module);


}

void js_readFile(const FunctionCallbackInfo<Value> &args) {
    // LOGI("js log:%d",args.Length());
    String::Utf8Value utf8(isolate_, args[0]);
    const char *cstr = ToCString(utf8);
    const char *source = openScriptFile(cstr);
    Local<String> js_src = String::NewFromUtf8(isolate_, source,
                                               NewStringType::kNormal).ToLocalChecked();
    args.GetReturnValue().Set(js_src);
}

void js_compileJSCode(const FunctionCallbackInfo<Value> &args) {
    // LOGI("js log:%d",args.Length());
    String::Utf8Value utf8(isolate_, args[0]);
    const char *cstr = ToCString(utf8);
    Local<String> js_src = String::NewFromUtf8(isolate_, cstr,
                                               NewStringType::kNormal).ToLocalChecked();
    Handle<Value> func = ExecuteJSScript3(isolate_, js_src, true, true);
    args.GetReturnValue().Set(func);
}

void jsInterfaceBind(){
    v8pp::context context;
//    isolate_ = context.isolate();
    isolate_ = context.isolate();
    v8::HandleScope scope(isolate_);
    // bind free variables and functions
    v8pp::module sun(isolate_);

    v8pp::class_<Person> Person_class(isolate_);
    Person_class
            .ctor<int , char const* >()
            .set("gender", &Person::gender)
            .set("getName", &Person::getName)
            .set("getAge", &Person::getAge)
            .set("setName", &Person::setName)
            .set("setAge", &Person::setAge);

    sun.set("Person", Person_class)
    .set("log",&js_log)
    .set("readFile",&js_readFile)
    .set("compileJSCode",&js_compileJSCode)
    .set("globalGamer",&GetGamer);

    isolate_->GetCurrentContext()-> Global()->Set(
            v8::String::NewFromUtf8(isolate_, "sun"), sun.new_instance());

    const char *source = openScriptFile("script/module.js");
    context.run_script(context.isolate(),source,true,true);
    Local<String> process_name =
            String::NewFromUtf8(isolate_, "runMain", NewStringType::kNormal)
                    .ToLocalChecked();
//    v8ppBindJs();
    Local<Value> process_val;
    if (!isolate_->GetCurrentContext()->Global()->Get(isolate_->GetCurrentContext(), process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()) {
        return;
    }
    Local<Function> process_fun = Local<Function>::Cast(process_val);
    function_.Reset(isolate_, process_fun);

    // Set up an exception handler before calling the Process function
    TryCatch try_catch(isolate_);

    const int argc = 0;

    Local<Value> argv[argc] = {};
    v8::Local<v8::Function> process =
            v8::Local<v8::Function>::New(isolate_, function_);
    Local<Value> result;
    if (!process->Call(isolate_->GetCurrentContext(), isolate_->GetCurrentContext()->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(isolate_, try_catch.Exception());
        LOGI("call js function error:%s",*error);
    }
}

void initV8() {
    LOGI("init v8");
    Platform *platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();

    jsInterfaceBind();
}


const char *executeJSFromString(const char *ch) {

    // Create a stack-allocated handle scope.
    HandleScope handle_scope(isolate_);
    // Create a new context.
    Local<Context> context = Context::New(isolate_);

    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);

    // Create a string containing the JavaScript source code.
    Local<String> source =
            String::NewFromUtf8(isolate_, ch,
                                NewStringType::kNormal)
                    .ToLocalChecked();

    // Compile the source code.
    Local<Script> script =
            Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    Local<Value> result = script->Run(context).ToLocalChecked();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(isolate_, result);
    LOGV("step v8 string:%s.", *utf8);
    // printf("%s\n", *utf8);
    return ToCString(utf8);
}

//call js function Game(gamer)
void executeJSFunction() {
    // Create a handle scope to keep the temporary object references.
    HandleScope handle_scope(isolate_);

    v8::Local<v8::Context> context =
            v8::Local<v8::Context>::New(isolate_, context_);
    // Enter this processor's context so all the remaining operations
    // take place there
    Context::Scope context_scope(context);

    // Set up an exception handler before calling the Process function
    TryCatch try_catch(isolate_);

//    const int argc = 1;
//    Local<Value> argv[argc] = {WrapGamerObject(isolate_, gamer)};
    const int argc = 0;
    Local<Value> argv[argc] = {};
    v8::Local<v8::Function> process =
            v8::Local<v8::Function>::New(isolate_, function_);
    Local<Value> result;
    if (!process->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(isolate_, try_catch.Exception());
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
    isolate_->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
}