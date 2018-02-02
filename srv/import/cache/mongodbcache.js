var Cache = require('js-cache');
var mongodb = require('mongodb');
var moment = require('moment');

var MongoDBCache = function (collectionName, url, options = {}) {
    this.collectionName = collectionName;
    this.dbUrl = url;
    this.dbOptions = options;
    this.setCacheHandler(new Cache({
        set: this.setHandler.bind(this),
        get: this.getHandler.bind(this),
        del: this.delHandler.bind(this),
        clear: this.clearHandler.bind(this),
    }));
}

MongoDBCache.prototype.setCacheHandler = function (ch) {
    this.cacheHandler = ch;
    return this;
}

MongoDBCache.prototype.setHandler = function (key, val, ttl) {
    var $this  = this;
    return new Promise(function (resolve, reject) {
        try {
            mongodb.connect($this.dbUrl, $this.dbOptions, function (err, db) {
                if (err) reject(err);
                else {
                    if (ttl) {
                        var lifetime = new Date();
                        lifetime.setSeconds(ttl);
                    }
                    else var lifetime = null;

                    db.collection($this.collectionName).updateOne(
                        {_id: key},
                        { '$set': {
                            v: new Buffer(JSON.stringify(val), 'binary'),
                            l: lifetime,
                        }},
                        { upsert: true },
                        function (err, res) {
                            if (err) reject(err);
                            else resolve(true);
                            db.close();
                        }
                    );
                }
            });
        }
        catch (err) {
            reject(err);
        }
    });
}

MongoDBCache.prototype.getHandler = function (key) {
    return new Promise(function (resolve, reject) {
        try {
            mongodb.connect(this.dbUrl, this.dbOptions, function (err, db) {
                if (err) reject(err);
                else {
                    db.collection(this.collectionName).findOne({
                        _id: key,
                    }, function (err, res) {
                        if (err) reject(err);
                        else {
                            if (res) {
                                if (res.l !== null && res.l <= new Date()) {
                                    resolve(false);
                                    // do delete
                                }
                                else {
                                    var result = JSON.parse(res.v);
                                    resolve(result);
                                    db.close();
                                }
                            }
                            else resolve(null);
                        }
                    });
                }
            }.bind(this));
        }
        catch (err) {
            reject(err);
        }
    }.bind(this));
}

MongoDBCache.prototype.delHandler = function (key) {
}
MongoDBCache.prototype.clearHandler = function (key) {
}

MongoDBCache.prototype.get = async function (key) {
    //let res = await this.cacheHandler.get(key);
    let res = await this.getHandler(key);
    return res;
}

MongoDBCache.prototype.set = function (key, val, ttl = 0) {
    return this.cacheHandler.set(key, val, ttl);
}

MongoDBCache.prototype.reset = function () {
}
module.exports = { MongoDBCache: MongoDBCache };
