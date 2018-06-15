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
  
    function resolve(request, options) {
      if (typeof request !== 'string') {
        throw new ERR_INVALID_ARG_TYPE('request', 'string', request);
      }
      return Module._resolveFilename(request, mod, false, options);
    }
  
    require.resolve = resolve;
  
    // function paths(request) {
    //   if (typeof request !== 'string') {
    //     throw new ERR_INVALID_ARG_TYPE('request', 'string', request);
    //   }
    //   return Module._resolveLookupPaths(request, mod, true);
    // }
  
    // resolve.paths = paths;
  
    // Enable support to add extra extension types.
    require.extensions = Module._extensions;
  
    require.cache = Module._cache;
  
    return require;
  }

function updateChildren(parent, child, scan) {
    var children = parent && parent.children;
    if (children && !(scan && children.includes(child)))
        children.push(child);
}

function Module(id, parent) {
    this.id = id;
    this.exports = {};
    this.parent = parent;
    // updateChildren(parent, this, false);
    this.filename = null;
    this.loaded = false;
    this.children = [];
}

Module._cache = Object.create(null);
Module._pathCache = Object.create(null);
Module._extensions = Object.create(null);
var modulePaths = [];
Module.globalPaths = [];

Module.wrap = function (script) {
    return Module.wrapper[0] + script + Module.wrapper[1];
};

Module.wrapper = [
    '(function (exports, require, module, __filename) { ',
    '\n});'
];

// Check the cache for the requested file.
// 1. If a module already exists in the cache: return its exports object.
// 2. Otherwise, create a new module for the file and save it to the cache.
//    Then have it load  the file contents before returning its exports
//    object.
Module._load = function (request, parent, isMain) {
    if (parent) {
        log('Module._load REQUEST %s parent: %s', request, parent.id);
    }

    var filename = Module._resolveFilename(request, parent, isMain);

    var cachedModule = Module._cache[filename];
    if (cachedModule) {
        // updateChildren(parent, cachedModule, true);
        return cachedModule.exports;
    }

    // Don't call updateChildren(), Module constructor already does.
    var module = new Module(filename, parent);

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
    var filename = "script/" + request + ".js";
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
    log('load %s for module %s', filename, this.id);

    // assert(!this.loaded);
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
    if (typeof id !== 'string') {
        throw new ERR_INVALID_ARG_TYPE('id', 'string', id);
    }
    if (id === '') {
        throw new ERR_INVALID_ARG_VALUE('id', id,
            'must be a non-empty string');
    }
    return Module._load(id, this, /* isMain */ false);
};


// Resolved path to process.argv[1] will be lazily placed here
// (needed for setting breakpoint when called with --inspect-brk)
var resolvedArgv;


// Run the file contents in the correct scope or sandbox. Expose
// the correct helper variables (require, module, exports) to
// the file.
// Returns exception, if any.
Module.prototype._compile = function (content, filename) {

    // content = stripShebang(content);

    // create wrapper function
    var wrapper = Module.wrap(content);

    var compiledWrapper = compileJSCode(wrapper);


    // var dirname = getDir(filename);
    var require = makeRequireFunction(this);

    var result;

    result = compiledWrapper(this.exports, require, this,
        filename);

    return result;
};


// Native extension for .js
Module._extensions['.js'] = function (module, filename) {
    var content = readFile(filename);
    module._compile(content, filename);
};

// bootstrap main module.
Module.runMain = function () {
    // Load the main module--the app main entrance.
    //  Module._load(process.argv[1], null, true);
    Module._load("main", null, true); // Currently fixed as main.js
};

// bootstrap main module.
function runMain(){
    // Load the main module--the app main entrance.
    //  Module._load(process.argv[1], null, true);
    Module._load("main", null, true); // Currently fixed as main.js
};