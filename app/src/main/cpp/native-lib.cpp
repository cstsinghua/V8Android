#include <jni.h>
#include <string>
#include <android/log.h>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include "include/v8-version-string.h"

#define TAG "V8Wrapper"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
/**
 * Must stub in case external snapshot files are used.
 */
namespace v8::internal {
    void ReadNatives() {}

    void DisposeNatives() {}

    void SetNativesFromFile(v8::StartupData *s) {}

    void SetSnapshotFromFile(v8::StartupData *s) {}
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_boyaa_v8wrapper_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    LOGV("Hello from C++");
    const char *ch;

    try {
        // Initialize V8.
//    v8::V8::InitializeICUDefaultLocation(nullptr);
//    v8::V8::InitializeExternalStartupData(nullptr);
        v8::Platform *platform = v8::platform::CreateDefaultPlatform();
        v8::V8::InitializePlatform(platform);
        v8::V8::Initialize();
        // Create a new Isolate and make it the current one.
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator =
                v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        //v8::Isolate* isolate = v8::Isolate::New(create_params);
        v8::Isolate* isolate = v8::Isolate::New(create_params);
        LOGV("step isolate");
        {
            v8::Isolate::Scope isolate_scope(isolate);

            // Create a stack-allocated handle scope.
            v8::HandleScope handle_scope(isolate);

            // Create a new context.
            v8::Local<v8::Context> context = v8::Context::New(isolate);
            LOGV("step context");

            // Enter the context for compiling and running the hello world script.
            v8::Context::Scope context_scope(context);
            LOGV("step scope");

            // Create a string containing the JavaScript source code.
            v8::Local<v8::String> source =
                    v8::String::NewFromUtf8(isolate, "'Hello' + ', World!'",
                                            v8::NewStringType::kNormal)
                            .ToLocalChecked();

            // Compile the source code.
            v8::Local<v8::Script> script =
                    v8::Script::Compile(context, source).ToLocalChecked();

            // Run the script to get the result.
            v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

            // Convert the result to an UTF8 string and print it.
            v8::String::Utf8Value utf8(isolate, result);
            ch = utf8.operator*();
            LOGV("step v8 string:%s.",ch);
            // printf("%s\n", *utf8);
        }

        // Dispose the isolate and tear down V8.
        isolate->Dispose();
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
        delete create_params.array_buffer_allocator;

    } catch (std::exception &e) {
        LOGV("%s", e.what());
    }

    LOGV("v8 version:%s",V8_VERSION_STRING);
    return env->NewStringUTF(ch);
    //return env->NewStringUTF(hello.c_str());
}
