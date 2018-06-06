//
// Created by CoulsonChen on 2018/6/5.
//

#ifndef V8ANDROID_GAME_H
#define V8ANDROID_GAME_H

#include <string>
#include "include/v8.h"

using namespace v8;

class Gamer {
public:
    Gamer(int age, std::string name);

    int getAge();

    std::string getName();

    void play();

    static void GetAge(Local<String> name,
                       const PropertyCallbackInfo<Value> &info);

    static void GetName(Local<String> name,
                        const PropertyCallbackInfo<Value> &info);

    static void getAgeByMethod(const FunctionCallbackInfo<Value> &args);

    static void getNameByMethod(const FunctionCallbackInfo<Value> &args);

    int age;
    std::string name;

};


#endif //V8ANDROID_GAME_H
