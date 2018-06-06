//
// Created by CoulsonChen on 2018/6/5.
//


#ifndef V8ANDROID_UTIL_H
#define V8ANDROID_UTIL_H

#include <jni.h>
#include "include/v8.h"
#include "Gamer.h"
using  namespace v8;

void setEnvAndAssetManager(JNIEnv *env, jobject assetManager);
char *openScriptFile(const char *path);
bool ExecuteJSScript(Isolate *isolate, const char *path, bool print_result,
                   bool report_exceptions);
void ReportException(Isolate *isolate, TryCatch *try_catch);
Local<ObjectTemplate> MakeGamerTemplate(
        Isolate* isolate);
Handle<Object> WrapGamerObject( Isolate* isolate,Gamer *gamer );
Gamer* UnwrapGamer(Local<Object> obj);



#endif //V8ANDROID_UTIL_H
