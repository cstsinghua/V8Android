exports = Module;

// Invoke with makeRequireFunction(module) where |module| is the Module object
// to use as the context for the require() function.
function makeRequireFunction(mod) {
    const Module = mod.constructor;
  
    function require(path) {
      try {
        exports.requireDepth += 1;
        return mod.require(path);
      } finally {
        exports.requireDepth -= 1;
      }
    }

    //有可能在创建模块之前使用到该函数，类似于静态函数
    function resolve(request, options) {
      if (typeof request !== 'string') {
        throw new ERR_INVALID_ARG_TYPE('request', 'string', request);
      }
      return Module._resolveFilename(request, mod, false, options);
    }
    require.resolve = resolve;
  
    // Enable support to add extra extension types.
    require.extensions = Module._extensions;
  
    require.cache = Module._cache;
  
    return require;
  }

function Module(id, parent, path) {
    this.id = id;
    this.exports = {};
    this.parent = parent;
    this.filename = null;
    this.loaded = false;
    this.path = path;
}

Module._cache = Object.create(null);
Module._pathCache = Object.create(null);
Module._extensions = Object.create(null);

Module.wrap = function (script) {
    return Module.wrapper[0] + script + Module.wrapper[1];
};

Module.wrapper = [
    '(function (exports, require, module, __filename) { ',
    '\n});'
];

Module._load = function (request, parent, isMain) {
    if (parent) {
        sun.log('Module._load REQUEST %s parent: %s  this:%s', request, parent.id, this.id);
    }
    var filename = Module._resolveFilename(request, parent, isMain);

    sun.log("file name === %s",filename);
    var cachedModule = Module._cache[filename];
    if (cachedModule) {
        return cachedModule.exports;
    }

    var module = new Module(filename, parent, this.path);
    sun.log("Module._load  module.id : %s",module.id);
    if (isMain) {
        module.id = '.';
    }

    Module._cache[filename] = module;
    tryModuleLoad(module, filename);

    return module.exports;
};

function tryModuleLoad(module, filename) {
    var threw = true;
    try {
        module.load(filename);
        threw = false;
    } finally {
        if (threw) {
            delete Module._cache[filename];
        }
    }
}

Module._resolveFilename = function (request, parent, isMain) {

    // look up the filename first, since that's the cache key.
    // var filename = Module._findPath(request, paths, isMain);
    var filePath;
    var filename
    if(isMain){
        filePath = request;
    }
    if(parent){
        while(request.indexOf("../") == 0){
            var index = parent.path.substring(0,parent.path.length-1).lastIndexOf("/");
            parent.path = parent.path.substring(0,index + 1);
            request = request.substring(3,request.length);
        }
        filePath = parent.path + request;
    }

    filename = filePath + ".js";
    var indexEnd = filePath.lastIndexOf("/");
    filePath = filePath.substring(0, indexEnd + 1);
    this.path = filePath;

    if (!filename) {
        // eslint-disable-next-line no-restricted-syntax
        var err = new Error(`Cannot find module '${request}'`);
        err.code = 'MODULE_NOT_FOUND';
        throw err;
    }
    return filename;
};


// Given a file name, pass it to the proper extension handler.
Module.prototype.load = function (filename) {
    this.filename = filename;
    // this.paths = Module._nodeModulePaths(path.dirname(filename));

    var extension = '.js';
    if (!Module._extensions[extension]) extension = '.js';
    Module._extensions[extension](this, filename);
    this.loaded = true;
};


// Loads a module at the given file path. Returns that module's
// `exports` property.
Module.prototype.require = function (id) {
    sun.log("require id = %s",id);
    if (typeof id !== 'string') {
        throw new ERR_INVALID_ARG_TYPE('id', 'string', id);
    }
    if (id === '') {
        throw new ERR_INVALID_ARG_VALUE('id', id,
            'must be a non-empty string');
    }

    return Module._load(id, this, /* isMain */ false);
};

Module.prototype._compile = function (content, filename) {
    var wrapper = Module.wrap(content);

    //字符串编译成js函数
    var compiledWrapper = sun.compileJSCode(wrapper);

    var require = makeRequireFunction(this);

    var result;

    //调用函数
    result = compiledWrapper(this.exports, require, this,
        filename);
    return result;
};

Module._extensions['.js'] = function (module, filename) {
    var content = sun.readFile(filename);
    module._compile(content, filename);
};


// bootstrap main module.
function runMain(){
    Module._load("script/main", null, true); // Currently fixed as main.js
};