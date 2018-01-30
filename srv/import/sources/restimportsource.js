var CacheableImportSource = require('../cacheableimportsource');
var NodeRestClient = require('node-rest-client').Client;
var Promise = require('promise');

var client = new NodeRestClient();

class RestImportSource extends CacheableImportSource {
    constructor(baseUrl, options, cache = null) {
//        super.constructor(options, cache);
        super(options, cache);
        this.baseUrl = baseUrl;
    }
    load (path, params = {}) {
        var url = this.makeUrl(path, this.mergeParams(params));
        console.log('Requesting: %s', url);
        return new Promise(function (fulfill, reject) {
            try {
                client.get(url, function (data, response) {
                    fulfill(data);
                });
            }
            catch (e) {
                reject(e);
            }
        });
    }
    makeUrl (path, params = {}) {
        if (Object.keys(params).length) {
            var qs = [];
            for (var i in params) {
                qs.push(i + '=' + params[i]);
            }
            return this.baseUrl + path + '?' + qs.join('&');
        }
        return this.baseUrl + path;
    }
    mergeParams(params = {}) {
        return Object.assign({}, params, this.getDefaultParams());
    }
}

module.exports = RestImportSource;
