var ImportSource = require('./importsource');
var Cache = require('js-cache');

class CacheableImportSource extends ImportSource {
    constructor (options = {}, cache = null) {
        super(options);
        if (cache) {
            this.setCacheHandler(cache);
        }
    }
    setCacheHandler (v) {
        if (mapped instanceof Cache) {
            this.cache = v;
        }
        else throw new Error('Cache handler must be an instance of Cache.');
    }
    async get (path, params = {}, cacheLifetime = 0, callback = null) {
        if (this.cache) { 
            var cacheKey = this.makeCacheKey(path, params);
            var cacheResult = null;
            if (cacheResult = this.cache.get(cacheKey)) {
                console.log(cacheResult);
                return cacheResult;
            }
        }
        var results = await super.get(path, params);

        if (results && this.cache) {
            this.cache.set(cacheKey, results, cacheLifetime);
        }
        return results;
    }
    makeCacheKey (path, params = {}) {
        var paramscopy = params;
        return path + JSON.stringify(paramscopy);
    }
}

module.exports = CacheableImportSource;
