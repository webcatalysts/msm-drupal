var Promise = require('promise');

CollectionProvider = function(databaseWrapper, databaseProvider, databaseName, collectionName) {
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.databaseName = databaseName || 'msm';
    this.collectionName = collectionName || 'msm_collections';
}

CollectionProvider.prototype.getCollection = async function(callback) {
    let con = await this.databaseWrapper.connect();
    var db = con.db(this.databaseName);
    return {
        con: con,
        db: db,
        col: db.collection(this.collectionName),
    };
}

CollectionProvider.prototype.find = async function(query = {}, project = null, sort = null, limit = null, skip = null) {
    let dbsrv = await this.getCollection();
    let cursor = dbsrv.col.find(query);
    if (project) {
        cursor.project(project);
    }
    if (limit) {
        cursor.limit(limit);
    }
    if (skip) {
        cursor.skip(skip);
    }
    if (sort) {
        console.log(sort);
        cursor.sort(sort);
    }
    let result = await cursor.toArray();
    dbsrv.con.close();
    return result;
}

CollectionProvider.prototype.findOne = async function (query = {}, options = {}) {
    let dbsrv = await this.getCollection();
    let doc = await dbsrv.col.findOne(query, options);
    dbsrv.con.close();
    return doc;
}

CollectionProvider.prototype.count = async function (query, options, callback) {
    let dbsrv = this.getCollection();
    let count = dbsrv.col.count(query, options);
    dbsrv.con.close();
    return count;
}

CollectionProvider.prototype.countDocuments = async function (colId, query = {}, options = {}) {
    let collection = await this.findOne({_id: colId});
    let dbsrv = await this.databaseWrapper.getCollection(collection.database, collection.collection);
    let count = await dbsrv.col.count(query, options);
    dbsrv.con.close();
    return count;
}

CollectionProvider.prototype.deleteOne = async function (query = {}) {
    var dbsrv = await this.getCollection();
    let result = await dbsrv.col.deleteOne(query, {});
    dbsrv.con.close();
    return result;
}

CollectionProvider.prototype.deleteMany = async function (query) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.deleteMany({database: dbName});
    dbsrv.con.close();
    return result;
}

CollectionProvider.prototype.save = async function(id, data, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.updateOne({_id: id}, data, {upsert:true});
    dbsrv.con.close();
    return result;
}

CollectionProvider.prototype.analyzeSchema = async function (collectionId, options = {}) {
    var schema = require('./schema');
    options = Object.assign({}, { merge: true }, options);
    let collection = await this.findOne({_id: collectionId});
    let con = await this.databaseWrapper.connect();

    let result = await schema.analyzeSchema(con, collection.database, collection.collection, options);
    let schemaResult = await schema.extractSchema(con, result.dbName, result.colName);
    if (collection.schema && options.merge) {
        //schemaResult = Object.assign(collection.schema, schemaResult);
        //schemaResult = schema.mergeSchema(collection.schema, schemaResult); 
        var sfields = schema.flattenSchemaFields(collection.schema);
        for (var f in sfields) {
            Object.assign(sfields[f], {
                percentContaining: 0,
                totalOccurrances: 0,
            });
        }
        schema.expandSchema(sfields,true);
        schemaResult = schema.mergeSchema(schemaResult, schema.expandSchema(sfields));
        //schemaResult = schema.mergeSchema(schemaResult, collection.schema); 
    }
    //return this.save(collection._id, { '$set': { 'schema': schema.sortSchema(schemaResult) } });
    return this.save(collection._id, { '$set': { 'schema': schemaResult, enabled: true } });
    return new Promise(function (fulfill, reject) {
        schema.analyzeSchema(con, collection.database, collection.collection, options)
            .then(function (result) {
                return schema.extractSchema(con, result.dbName, result.colName);
            })
            .then(function (schema) {
                con.close();
                return this.save(collection._id, { "$set": { "schema": schema }, "enabled": true, });
            })
            .then(function (saveResult) {
                fulfill(saveResult);
            })
            .catch(reject);
    });
}

CollectionProvider.prototype.query = async function (collectionId, params = {}) {
    params = Object.assign({}, {
        query: {},
        project: false,
        skip: false,
        limit: false,
        batchSize: false,
        sort: false,
    }, params);

    let collection = await this.findOne({_id: collectionId});
    if (!collection) {
        return { ok: 0, error: "Collection does not exist." };
    }
    if (!collection.schema) {
        return { ok: 0, error: "Collection does not have schema." };
    }

    let con = await this.databaseWrapper.connect();
    var db = con.db(collection.database);
    var col = db.collection(collection.collection);
    var result = {
        schema: collection.schema,
    };

    if (collection.preExecute) {
        eval('var preExecute = async function (document, collection, params, result) { ' + collection.preExecute + ' }');
        await preExecute(collection, col, params, result);
    }

    var cursor = col.find(params.query);

    result.totalResults = await cursor.count(false);
    if (result.totalResults) {
        if (params.project) {
            cursor.project(params.project);
            var schema = require('./schema');
            result.schema = schema.projectSchema(params.project, result.schema);
        }
        if (params.sort) {
            console.log(params.sort);
            cursor.sort(params.sort);
        }
        if (params.skip) {
            cursor.skip(1*params.skip);
        }
        if (params.limit) {
            cursor.limit(1*params.limit);
        }
        if (params.batchSize) {
            cursor.batchSize(1*params.batchSize);
        }
        result.results = await cursor.toArray();
        result.numResults = result.results.length;

        if (collection.postExecute) {
            eval('var postExecute = function (document, collection, params, result) { ' + collection.postExecute + ' }');
            postExecute(collection, col, params, result);
        }
        result.ok = 1;
        con.close();
        return result;
    }
}

CollectionProvider.prototype.resetSchema = async function (databaseName, collectionName, callback) {
    let result = await this.save(databaseName, collectionName, { "$set": { enabled: false }, "$unset": { schema: "" } });
    return result;
}

CollectionProvider.makeId = function (databaseName, collectionName) {
    return databaseName + '.' + collectionName;
}

exports.CollectionProvider = CollectionProvider;
exports.makeId = CollectionProvider.makeId;
