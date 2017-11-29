
var assert = require('assert'),
    promise = require('promise'),
    CollectionProvider = require('./collectionprovider');

DatabaseProvider = function (databaseWrapper, databaseName, collectionName) {
    this.databaseWrapper = databaseWrapper;
    this.databaseName = databaseName || 'msm';
    this.collectionName = collectionName || 'msm_databases';
}

DatabaseProvider.prototype.setCollectionProvider = function(collectionProvider) {
    this.collectionProvider = collectionProvider;
}

DatabaseProvider.prototype.getCollection = function(callback) {
    var provider = this;
    this.databaseWrapper.connect(function(err, con) {
        if (err) callback(err);
        else {
            var db = con.db(provider.databaseName);
            callback(null, db.collection(provider.collectionName), db, con);
        }
    });
}

DatabaseProvider.prototype.findAll = function (callback) {
    var mergeDbs = function(documents, existing) {
        var results = [];
        // key the existing dbs
        var numExisting = existing.length;
        var keyedExisting = {};
        for (var i = 0; i < numExisting; i++) {
            var db = existing[i];
            keyedExisting[db.name] = 1;
        }

        // key the documents
        var keyedDocs = {};
        var numDocuments = documents.length;
        for (var i = 0; i < numDocuments; i++) {
            var doc = documents[i];
            if (!keyedExisting[doc._id]) {
                doc.name = doc._id;
                doc.sizeOnDisk = 0;
                doc.empty = true;
                delete doc._id;
                results.push(doc);
            }
            else {
                keyedDocs[doc._id] = doc;
                delete keyedDocs[doc._id]._id;
            }
        }
        for (var i = 0; i < numExisting; i++) {
            var db = existing[i];
            if (db.name == 'admin' || db.name == 'local') continue;
            if (keyedDocs[db.name]) {
                db = Object.assign(keyedDocs[db.name], db);
                db = keyedDocs[db.name];
            }
            else db.enabled = false;
            results.push(db);
        }
        return results;
    }
    this.getCollection(function(err, col, db, con) {
        con.db('admin').admin().listDatabases(function(err, existingDbs) {
            if (err) callback(err);
            else {
                col.find().toArray(function(err, docs) {
                    con.close();
                    callback(null, mergeDbs(docs, existingDbs.databases));
                });
            }
        });
    });
}

DatabaseProvider.prototype.findById = function(id, callback) {
    var mergeCollections = function(existing, docs) {
        var existingKeyed = {};
        var docsKeyed = {};
        var numExisting = existing.length;
        var numDocs = docs.length;
        var results = [];
        for (var i = 0; i < numExisting; i++) {
            var c = existing[i];
            existingKeyed[CollectionProvider.makeId(id, c.name)] = c;
        }
        for (var i = 0; i < numDocs; i++) {
            var d = docs[i];
            if (existingKeyed[d._id]) {
                docsKeyed[d._id] = d;
            }
            else {
                results.push(d);
            }
        }
        for (var i = 0; i < numExisting; i++) {
            var c = existing[i];
            var cid = CollectionProvider.makeId(id, c.name);
            if (docsKeyed[cid]) {
                var d = docsKeyed[cid];
                d.collectionInfo = c;
                results.push(d);
            }
            else {
                var d = {
                    _id: cid,
                    name: 'Uninitialized: ' + c.name,
                    collection: c.name,
                    database: id,
                    enabled: false,
                    collectionInfo: c,
                }
                delete d.collectionInfo.name;
                results.push(d);
            }
        }
        return results;
    }
    var provider = this;
    this.getCollection(function(err, col, db, con) {
        col.findOne({_id: id}, {}, function(err, doc) {
            if (err) callback(err);
            else if (!doc) {
                con.close();
                callback(null, {});
            }
            else {
                doc.name = doc._id;
                delete doc._id;
                con.db(doc.name).listCollections().toArray(function(err, cols) {
                    con.close();
                    var existingCollections = cols;
                    provider.collectionProvider.findByDatabase(doc.name, function(err, cols) {
                        doc.collections = mergeCollections(existingCollections, cols);
                        callback(null, doc);
                    });
                });
            }
        });
    });
}

DatabaseProvider.prototype.save = function(name, data, callback) {
    this.getCollection(function(err, collection) {
        if (err) callback(err);
        else {
            collection.update({_id: name}, data, {upsert: true}, function (err, res) {
                if (err) callback(err);
                else callback(null, res);
            });
        }
    });
}

DatabaseProvider.prototype.delete = function(name, callback) {
    var collectionProvider = this.collectionProvider;
    var databaseProvider = this;
    collectionProvider.deleteByDatabase(name, function (err, result) {
        if (err) callback(err);
        else {
            databaseProvider.getCollection(function(err, collection) {
                if (err) callback(err);
                else {
                    collection.deleteOne({_id: name}, function (err, result) {
                        if (err) callback(err);
                        else callback(null, result);
                    });
                }
            });
        }
    });
}

DatabaseProvider.prototype.disable = function(name, callback) {
    this.getCollection(function(err, collection) {
        collection.updateOne({ _id: name }, { $set: { enabled: false }}, { upsert: true }, callback);
    });
}

DatabaseProvider.prototype.enable = function(name, callback) {
    this.getCollection(function(err, collection) {
        collection.updateOne({ _id: name }, { $set: { enabled: true }}, { upsert: true }, callback);
    });
}

exports.DatabaseProvider = DatabaseProvider;
