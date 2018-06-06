//
// Created by CoulsonChen on 2018/6/5.
//
#include <android/asset_manager_jni.h>
#include "log.h"
#include "util.h"
#include "Gamer.h"

static JNIEnv *sEnv = NULL;
static jobject sAssetManager = NULL;
static Global<ObjectTemplate> gamer_template_;

void setEnvAndAssetManager(JNIEnv *env, jobject assetManager) {
    sEnv = env;
    //sAssetManager = assetManager; // this will cause this problem:
    // E/dalvikvm: JNI ERROR (app bug): attempt to use stale local reference
    /**
     * Since android 4.0 garbage collector was changed. Now it moves object around during garbage collection,
     * which can cause a lot of problems.
     * Imagine that you have a static variable pointing to an object, and then this object gets moved by gc.
     * Since android uses direct pointers for java objects, this would mean that your static variable is now pointing
     * to a random address in the memory, unoccupied by any object or occupied by an object of different sort.
     * This will almost guarantee that you'll get EXC_BAD_ACCESS next time you use this variable.
     * So android gives you JNI ERROR (app bug) error to prevent you from getting undebugable EXC_BAD_ACCESS.
     * Now there are two ways to avoid this error.
     *      1.You can set targetSdkVersion in your manifest to version 11 or less.
     *        This will enable JNI bug compatibility mode and prevent any problems altogether.
     *        This is the reason why your old examples are working.
     *      2.You can avoid using static variables pointing to java objects or
     *        make jobject references global before storing them by calling env->NewGlobalRef(ref).
     *        Perhaps on of the biggest examples here is keeping jclass objects.
     *        Normally, you'll initialize static jclass variable during JNI_OnLoad,
     *        since class objects remain in the memory as long as the application is running.
     *
     * This code will lead to a crash:
     *      static jclass myClass;
     *      JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM * vm, void * reserved) {
     *          myClass = env->FindClass("com/example/company/MyClass");
     *          return JNI_VERSION_1_6;
     *      }
     *
     * While this code will run fine:
     *      static jclass myClass;
     *      JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM * vm, void * reserved) {
     *          jclass tmp = env->FindClass("com/example/company/MyClass");
     *          myClass = (jclass)env->NewGlobalRef(tmp);
     *          return JNI_VERSION_1_6;
     *      }
     */
    sAssetManager = env->NewGlobalRef(assetManager);
}

static AAsset *loadAsset(const char *path) {
    AAssetManager *nativeManager = AAssetManager_fromJava(sEnv, sAssetManager);
    if (nativeManager == NULL) {
        return NULL;
    }
    return AAssetManager_open(nativeManager, path, AASSET_MODE_UNKNOWN);
}

char *openScriptFile(const char *path) {
    AAsset *asset = loadAsset(path);
    if (asset == NULL) {
        LOGE("Couldn't load %s", path);
        return NULL;
    }
    off_t length = AAsset_getLength(asset);
    char *buffer = new char[length + 1];
    int num = AAsset_read(asset, buffer, length);
    AAsset_close(asset);
    if (num != length) {
        LOGE("Couldn't read %s", path);
        delete[] buffer;
        return NULL;
    }
    buffer[length] = '\0';
    return buffer;
}

//execute javascript source code
bool ExecuteJSScript(Isolate *isolate, const char *path, bool print_result,
                     bool report_exceptions) {

    LOGI("ExecuteScript");
    // Create filename

    char *jsChar = openScriptFile(path);

    HandleScope handle_scope(isolate);

    Local<String> source = String::NewFromUtf8(isolate, jsChar,
                                               NewStringType::kNormal).ToLocalChecked();

    TryCatch try_catch(isolate);

    Local<Context> context(isolate->GetCurrentContext());
    Local<Script> script;
    if (!Script::Compile(context, source).ToLocal(&script)) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ReportException(isolate, &try_catch);
        return false;
    } else {
        Local<Value> result;
        if (!script->Run(context).ToLocal(&result)) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ReportException(isolate, &try_catch);
            return false;
        } else {
            assert(!try_catch.HasCaught());
            if (print_result && !result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                String::Utf8Value str(isolate, result);
                LOGI("===========\n%s\n", *str);
            }
            return true;
        }
    }
}

void ReportException(Isolate *isolate, TryCatch *try_catch) {
    HandleScope handle_scope(isolate);
    String::Utf8Value exception(isolate, try_catch->Exception());
//    const char *exception_string = ToCString(exception);
    Local<Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        LOGI("exception_string ; %s\n", *exception);
    } else {
        // Print (filename):(line number): (message).
        String::Utf8Value filename(isolate,
                                   message->GetScriptOrigin().ResourceName());
        Local<Context> context(isolate->GetCurrentContext());
        int linenum = message->GetLineNumber(context).FromJust();
        LOGI("exception_string : %s:%i: %s\n", *filename, linenum, *exception);
        // Print line of source code.
        String::Utf8Value sourceline(
                isolate, message->GetSourceLine(context).ToLocalChecked());
        LOGI("stderr :%s\n", *sourceline);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        Local<Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
            stack_trace_string->IsString() &&
            Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
            String::Utf8Value stack_trace(isolate, stack_trace_string);
            LOGI("exception_string : %s\n\n", *stack_trace);
        }
    }

}

Local<ObjectTemplate> MakeGamerTemplate(
        Isolate *isolate) {
    EscapableHandleScope handle_scope(isolate);
    Local<ObjectTemplate> result = ObjectTemplate::New(isolate);
    result->SetInternalFieldCount(1);
    // Add accessors for each of the fields of the gamer.
    result->SetAccessor(
            String::NewFromUtf8(isolate, "age", NewStringType::kInternalized)
                    .ToLocalChecked(),
            Gamer::GetAge);
    result->SetAccessor(
            String::NewFromUtf8(isolate, "name", NewStringType::kInternalized)
                    .ToLocalChecked(),
            Gamer::GetName);
    // Add function for each of the instance function of the gamer.
    result->Set(String::NewFromUtf8(isolate, "getAge", NewStringType::kNormal)
                        .ToLocalChecked(),
                FunctionTemplate::New(isolate, Gamer::getAgeByMethod));
    result->Set(String::NewFromUtf8(isolate, "getName", NewStringType::kNormal)
                        .ToLocalChecked(),
                FunctionTemplate::New(isolate, Gamer::getNameByMethod));

    // Again, return the result through the current handle scope.
    return handle_scope.Escape(result);
}

//This will expose an object with the type Game, into the global scope.
//It will return a handle to the JS object that represents this c++ instance.
Handle<Object> WrapGamerObject(Isolate *isolate, Gamer *gamer) {
    // We will be creating temporary handles so we use a handle scope.
    EscapableHandleScope handle_scope(isolate);

    /// Fetch the template for creating JavaScript http request wrappers.
    // It only has to be created once, which we do on demand.
    if (gamer_template_.IsEmpty()) {
        Local<ObjectTemplate> raw_template = MakeGamerTemplate(isolate);
        gamer_template_.Reset(isolate, raw_template);
    }
    Local<ObjectTemplate> templ =
            Local<ObjectTemplate>::New(isolate, gamer_template_);

    // Create an empty gamer wrapper.
    Local<Object> result =
            templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
    // Wrap the raw C++ pointer in an External so it can be referenced
    // from within JavaScript.
    Local<External> gamer_ptr = External::New(isolate, gamer);
    // Store the request pointer in the JavaScript wrapper.
    result->SetInternalField(0, gamer_ptr);
    // Return the result through the current handle scope.  Since each
    // of these handles will go away when the handle scope is deleted
    // we need to call Close to let one, the result, escape into the
    // outer handle scope.
    return handle_scope.Escape(result);
}

Gamer *UnwrapGamer(Local<Object> obj) {
    Local<External> field = Local<External>::Cast(obj->GetInternalField(0));
    void *ptr = field->Value();
    return static_cast<Gamer *>(ptr);
}
