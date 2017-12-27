var Promise = require('promise');

var CollectionProcess = function (collectionId, databaseWrapper, databaseProvider, collectionProvider) {
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.collectionProvider = collectionProvider;
    this.collectionId = collectionId;
}

CollectionProcess.prototype.run = async function () {
    this.collection = await this.collectionProvider.findOne({_id: this.collectionId});
    if (!this.collection.source) {
        throw new Error('Collection %s missing source attribute.', this.collection._id);
    }
    this.source = await this.collectionProvider.findOne({_id: this.collection.source});
    if (!this.collection.source) {
        throw new Error('Source collection %s not found.', this.collect.source);
    }
    var numSourceDocuments = await this.collectionProvider.countDocuments(this.collection.source);
    if (numSourceDocuments <= 0) {
        throw new Error('Source collection %s is empty.');
    }

    switch (this.collection.type) {
        case 'mapReduce':
            var handler = CollectionProcessHandlerMapReduce;
            break;
        case 'aggregation':
            var handler = CollectionProcessHandlerAggregation;
            break;
        case 'custom':
            var handler = CollectionProcessHandlerCustom;
            break;
        default:
            throw new Error('Unknown collection type or invalid collection.');
    }

    var result = {};
    if (this.collection.preProcess) {
        console.log('Pre-processing: %s', this.collection._id);
        eval('var preProcess = async function(instance, result) { ' + this.collection.preProcess + '};');
        await preProcess(this, result); 
    }
    console.log('Processing collection: %s', this.collection._id);
    await handler(this, result);
    console.log('Successfully processed collection: ', this.collection._id);
    if (this.collection.postProcess) {
        console.log('Post-processing: %s', this.collection._id);
        eval('var postProcess = function(instance, result) { ' + this.collection.postProcess + '};');
        await postProcess(this, result);
    }
}

var ProcessMapReduce = function (instance, callback) {
}

//var ProcessCustom = function (instance, callback) {
async function CollectionProcessHandlerCustom (instance, result) {
    console.log('Processing custom eval collection: ', instance.collection._id);
    let con = await instance.databaseWrapper.connect();
    var sourceDatabaseName = instance.source.database;
    var sourceDatabase = con.db(sourceDatabaseName);
    var sourceCollectionName = instance.source.collection;
    var sourceCollection = sourceDatabase.collection(sourceCollectionName);
    if (instance.collection.persist) {
        var destinationDatabaseName = instance.collection.database;
        var destinationDatabase = con.db(destinationDatabaseName);
        var destinationCollectionName = instance.collection.collection;
        var destinationCollection = destinationDatabase.collection(destinationCollectionName);
    }
    if (instance.collection.dependencies && instance.collection.dependencies.length) {
        let deps = await instance.collectionProvider.find({_id: { "$in": instance.collection.dependencies }});
        var numD = deps.length;
        var dependencies = {};
        for (var d = 0; d < numD; d++) {
            var dep = deps[d];
            dependencies[dep._id] = {
                databaseName: dep.database,
                collectionName: dep.collection,
                database: con.db(dep.database),
                collection: con.db(dep.database).collection(dep.collection),
            }
        }
    }
    eval('var evalCustom = async function () { ' + instance.collection.eval.code + ' };');
    await evalCustom();
    con.close();
    return true;
}

module.exports = {
    CollectionProcess: CollectionProcess,
}
