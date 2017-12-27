
var assert = require('assert'),
    promise = require('promise');

DatabaseProvider = function (databaseWrapper, databaseName, collectionName) {
    this.databaseWrapper = databaseWrapper;
    this.databaseName = databaseName || 'msm';
    this.collectionName = collectionName || 'msm_databases';
}

DatabaseProvider.prototype.setCollectionProvider = function(collectionProvider) {
    this.collectionProvider = collectionProvider;
}

DatabaseProvider.prototype.getCollection = async function() {
    return await this.databaseWrapper.getCollection(this.databaseName, this.collectionName);
}

DatabaseProvider.prototype.find = async function (query = {}) {
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

    let dbsrv = await this.getCollection();
    let existingDbs = await dbsrv.con.db('admin').admin().listDatabases();
    let docs = await dbsrv.col.find(query).toArray();
    dbsrv.con.close();
    return mergeDbs(docs, existingDbs.databases);
}

DatabaseProvider.prototype.findOne = async function(query, callback) {
    var mergeCollections = function(id, existing, docs) {
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

    let dbsrv = await this.getCollection();
    let doc = await dbsrv.col.findOne(query);
    doc.name = doc._id;
    delete doc._id;
    let existingCollections = await dbsrv.con.db(doc.name).listCollections().toArray();
    dbsrv.con.close();
    let cols = await this.collectionProvider.find({database: doc.name});
    doc.collections = mergeCollections(doc.name, existingCollections, cols);
    return doc;
}

DatabaseProvider.prototype.save = async function (name, data, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.update({_id: name}, data, {upsert: true});
    return result;
}

DatabaseProvider.prototype.delete = async function (name, callback) {
    let res1 = await this.collectionProvider.delete({database: name});
    let res2 = await this.databaseProvider.deleteOne({_id: name});
    return res2;
}

DatabaseProvider.prototype.disable = async function(name, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.updateOne({ _id: name }, { $set: { enabled: false }}, { upsert: true });
    return result;
}

DatabaseProvider.prototype.enable = async function(name, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.updateOne({ _id: name }, { $set: { enabled: true }}, { upsert: true });
    return result;
}

DatabaseProvider.prototype.analyzeSchema = async function (dbName, colName, options = {}) {
    var schema = require('./schema');
    let con = await this.databaseWrapper.connect();
    var colId = dbName + '.' + colName;
    await this.collectionProvider.save(colId, { '$set': {
        name: colId,
        collection: colName,
        database: dbName,
        enabled: false,
        analyzingSchema: true
    }});

    let result = await schema.analyzeSchema(con, dbName, colName, options);
    let schemaResult = await schema.extractSchema(con, result.dbName, result.colName);
    return this.save(colId, {
        '$set': {
            'schema': schemaResult,
            'enabled': true,
            'schemaImported': true,
        },
        '$unset': { 'analyzingSchema': '' },
    });
}

exports.DatabaseProvider = DatabaseProvider;
