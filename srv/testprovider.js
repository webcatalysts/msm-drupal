
var assert = require('assert'),
    promise = require('promise');

TestProvider = function (databaseWrapper, databaseName, collectionName) {
    this.databaseWrapper = databaseWrapper;
    this.databaseName = databaseName || 'msm';
    this.collectionName = collectionName || 'msm_tests';
}

TestProvider.prototype.getCollection = async function() {
    return await this.databaseWrapper.getCollection(this.databaseName, this.collectionName);
}

TestProvider.prototype.getDatabase = async function () {
    return await this.databaseWrapper.getDatabase(this.databaseName);
}

TestProvider.prototype.find = async function (query = {}) {
    let dbsrv = await this.getCollection();
    let existingDbs = await dbsrv.con.db('admin').admin().listDatabases();
    let docs = await dbsrv.col.find(query).toArray();
    dbsrv.con.close();
    return docs;
}

TestProvider.prototype.findOne = async function(query, callback) {
    let dbsrv = await this.getCollection();
    let doc = await dbsrv.col.findOne(query);
    dbsrv.con.close();
    return doc;
}

TestProvider.prototype.save = async function (id, data, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.update({_id: id}, data, {upsert: true});
    dbsrv.con.close();
    return result;
}

TestProvider.prototype.delete = async function (id, callback) {
    let dbsrv = await this.getCollection();
    let res = await dbsrv.col.remove({_id:id});
    return res;
}

TestProvider.prototype.disable = async function(id, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.updateOne({ _id: id }, { $set: { enabled: false }}, { upsert: true });
    dbsrv.con.close();
    return result;
}

TestProvider.prototype.enable = async function(id, callback) {
    let dbsrv = await this.getCollection();
    let result = await dbsrv.col.updateOne({ _id: id }, { $set: { enabled: true }}, { upsert: true });
    dbsrv.con.close();
    return result;
}
TestProvider.prototype.runTest = async function (id, options = {}) {
    let testDoc = await this.findOne({_id: id});
    let testResult = { last: new Date };
    if (testDoc) {
        console.log('Running test: %s', id);
        await this.databaseWrapper.connect();
        var db = this.databaseWrapper.connection.db(testDoc.database);
        var col = db.collection(testDoc.collection);
        var numU = testDoc.units.length;
        for (var i = 0; i < numU; i++) {
            var result = await this.evalTestUnit(testDoc.units[i], testDoc, col, this.databaseWrapper.connection, db, options);
            if (result) {
                console.log('Fail: %s', result);
                testResult.error = result;
                testResult.pass = false;
                testResult.unitFailed = i;
                await this.save(testDoc._id, { '$set': testResult });
                return testResult;
            }
        }
        testResult.pass = true;
        await this.save(testDoc._id, {'$set': testResult, '$unset': { error: "", unitFailed: "" }});
        console.log('Finished test: %s', id);
        return testResult;
    }
}
TestProvider.prototype.evalTestUnit = async function (testUnit, testDoc, col, con, db, options = {}) {
    try {
        eval('var evalTestUnit = async function (testDoc, col, con, db) {' + testUnit.code + '; con.close(); };');
        await evalTestUnit(testDoc, col, con, db);
    }
    catch (err) {
        return err.message;
    }
    return null;
}

TestProvider.prototype.runAllTests = async function (query = {}, options = {}) {
    let dbsrv = await this.getCollection();
    let testDocs = await dbsrv.col.find(query, {_id: 1}).toArray();
    dbsrv.con.close();
    numTestDocs = testDocs.length;
    for (var t = 0; t < numTestDocs; t++) {
        this.runTest(testDocs[t]._id);
    }
}

exports.TestProvider = TestProvider;
