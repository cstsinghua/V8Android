#include "v8pp/json.hpp"

namespace v8pp {

V8PP_IMPL std::string json_str(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
	if (value.IsEmpty())
	{
		return std::string();
	}

	v8::HandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate, "JSON", v8::NewStringType::kNormal).ToLocalChecked();
	v8::Local<v8::Object> json = context->Global()->Get(context, key).ToLocalChecked().As<v8::Object>();

	key = v8::String::NewFromUtf8(isolate, "stringify", v8::NewStringType::kNormal).ToLocalChecked();
	v8::Local<v8::Function> stringify = json->Get(context, key).ToLocalChecked().As<v8::Function>();

	v8::Local<v8::Value> result = stringify->Call(context, json, 1, &value).ToLocalChecked();
	v8::String::Utf8Value const str(result);

	return std::string(*str, str.length());
}

V8PP_IMPL v8::Local<v8::Value> json_parse(v8::Isolate* isolate, std::string const& str)
{
	if (str.empty())
	{
		return v8::Local<v8::Value>();
	}

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::Local<v8::String> key = v8::String::NewFromUtf8(isolate, "JSON", v8::NewStringType::kNormal).ToLocalChecked();
	v8::Local<v8::Object> json = context->Global()->Get(context, key).ToLocalChecked().As<v8::Object>();

	key = v8::String::NewFromUtf8(isolate, "parse", v8::NewStringType::kNormal).ToLocalChecked();
	v8::Local<v8::Function> parse = json->Get(context, key).ToLocalChecked().As<v8::Function>();

	v8::Local<v8::Value> value = v8::String::NewFromUtf8(isolate, str.data(),
		v8::NewStringType::kNormal, static_cast<int>(str.size())).ToLocalChecked();

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> result;
	parse->Call(context, json, 1, &value).ToLocal(&result);
	if (try_catch.HasCaught())
	{
		result = try_catch.Exception();
	}
	return scope.Escape(result);
}

} // namespace v8pp
