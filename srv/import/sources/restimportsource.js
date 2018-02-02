var CacheableImportSource = require('../cacheableimportsource'),
    NodeRestClient = require('node-rest-client-promise').Client,
    Promise = require('promise')
    QueryString = require('querystring');

var client = new NodeRestClient();

class RestImportSource extends CacheableImportSource {

    constructor(baseUrl, options, cache = null) {
//        super.constructor(options, cache);
        super(Object.assign({maxRetries: 3}, options), cache);
        this.baseUrl = baseUrl;
    }
    async load (path, params = {}, retries = 0) {
        console.log(retries);
        var url = this.makeUrl(path, this.mergeParams(params));
        console.log('Requesting: %s', url);
        try {
            var result = await client.getPromise(url);
            this.validateResponse(result.response, result.data, url);
            return result.data;
        }
        catch (err) {
            if (this.shouldRetry(err, result, retries, this.options.maxRetries)) {
                return await this.load(path, params, ++retries);
            }
            else throw Error(err);
        }
    }
    shouldRetry(error, result, retries, maxRetries) {
        if (retries >= maxRetries) return false;
        var { statusCode } = result.response;
        if ([404].indexOf(statusCode) < 0) {
            return true;
        }
        return false;
    }
    validateResponse (res, data, url) {
        const { statusCode } = res;
        if (statusCode !== 200) {
            let errorMessage = res.statusMessage || '';
            throw new Error(`Request to ${url} failed.\n`
                + `Status Code: ${statusCode}\n`
                + `Error Message: ${errorMessage}\n`);
        }
    }
    makeUrl (path, params = {}) {
        if (Object.keys(params).length) {
            return this.baseUrl + path + '?' + QueryString.stringify(params);
        }
        return this.baseUrl + path;
    }
    makeCacheKey (path, params = {}) {
        return this.makeUrl(path, params);
    }
}

module.exports = RestImportSource;
