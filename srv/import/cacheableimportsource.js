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
        this.cache = v;
        return;
        if (v instanceof Cache) {
            this.cache = v;
        }
        else {
            console.log(v);
            throw new Error('Cache handler must be an instance of Cache. Received "' + typeof v + '" instead.');
        }
    }
    async get (path, params = {}, cacheLifetime = 0, skipCache = false) {
        params = this.mergeParams(params);
        if (!skipCache && cacheLifetime !== null && this.cache) { 
            var cacheKey = this.makeCacheKey(path, params);
            let cacheResult = await this.cache.get(cacheKey);
            if (cacheResult) {
                console.log('Cache: HIT ' + cacheKey);
                console.log(cacheResult);
                return cacheResult;
            }
            console.log('Cache: MISS');
        }

        var results = await super.get(path, params);

        if (cacheLifetime !== null && results && this.cache) {
            console.log('setting cache: ' + cacheLifetime);
            var cacheResult = JSON.parse(JSON.stringify(results));
            await this.cache.set(cacheKey, cacheResult, cacheLifetime, function (err) {
                if (err) throw new Error(err);
                else console.log('cache success?!?');
            });
        }
        return results;
    }
    makeCacheKey (path, params = {}) {
        if (Object.keys(params).length) {
            var paramscopy = params;
            return path + JSON.stringify(paramscopy);
        }
        return path;
    }
}

module.exports = CacheableImportSource;
