//
// Created by CoulsonChen on 2018/6/5.
//

#include <string>
#include "Gamer.h"
#include "log.h"
#include "util.h"

Gamer::Gamer(int age, std::string name) : age(age), name(name) {

}

void Gamer::GetAge(Local<String> name, const PropertyCallbackInfo<Value> &info) {
    // Extract the C++ gamer object from the JavaScript wrapper.
    Gamer *gamer = UnwrapGamer(info.Holder());

    // Wrap the result in a JavaScript Integer and return it.
    info.GetReturnValue().Set(gamer->age);
}

void Gamer::GetName(Local<String> name, const PropertyCallbackInfo<Value> &info) {
    // Extract the C++ gamer object from the JavaScript wrapper.
    Gamer *gamer = UnwrapGamer(info.Holder());

    // Wrap the result in a JavaScript String and return it.
    info.GetReturnValue().Set(String::NewFromUtf8(info.GetIsolate(), (gamer->name).c_str(),
                                                  NewStringType::kNormal).ToLocalChecked());

}

int Gamer::getAge() {
    return age + 10;
}

std::string Gamer::getName() {
    std::string str = "hello ";
    return str.append(name);
}

void Gamer::play() {
    LOGI("gamer plays game");
}

void Gamer::getAgeByMethod(const FunctionCallbackInfo<Value> &args) {
    // Extract the C++ gamer object from the JavaScript wrapper.
    Gamer *gamer = UnwrapGamer(args.Holder());
    args.GetReturnValue().Set(gamer->getAge());

}

void Gamer::getNameByMethod(const FunctionCallbackInfo<Value> &args) {
    // Extract the C++ gamer object from the JavaScript wrapper.
    Gamer *gamer = UnwrapGamer(args.Holder());
    args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), gamer->getName().c_str(),
                                                  NewStringType::kNormal).ToLocalChecked());
}

void GetGamer(Local<String> name, const PropertyCallbackInfo<Value> &info) {
    // Wrap the result in a JavaScript String and return it.
    info.GetReturnValue().Set(WrapGamerObject(info.GetIsolate(), new Gamer(100, "zhangsanfeng")));
}
