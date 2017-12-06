

var CollectionProcessInstance = function (collection, databaseWrapper, databaseProvider, collectionProvider) {
    this.collection = collection;
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.collectionProvider = collectionProvider;
}

var ProcessCollection = function (instance, callback) {
    var result = {};
    switch (instance.collection.type) {
        case 'mapReduce':
            var handler = ProcessMapReduce;
            break;
        case 'aggregation':
            var handler = ProcessAggregation;
            break;
        case 'custom':
            var handler = ProcessCustom;
            break;
        default:
            var handler = function () { callback(null, { ok: 1 }); }
            break;
    }
    if (instance.collection.preProcess) {
        eval('var preProcess = function() { ' + instance.collection.preProcess + '};');
    }
    if (instance.collection.postProcess) {
        eval('var postProcess = function() { ' + instance.collection.postProcess + '};');
    }
    instance.collectionProvider.findById(instance.collection.source, function (err, source) {
        instance.source = source;
        handler(instance, function () {});
    });
    callback(null, { ok: 0 });
}

var ProcessMapReduce = function (instance, callback) {
}

var ProcessCustom = function (instance, callback) {
    instance.databaseWrapper.connect(function (err, con) {
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
        console.log('eval custom');
        eval('var evalCustom = function () { ' + instance.collection.eval.code + ' };');
        evalCustom();
    });
}

module.exports = {
    createInstance: CollectionProcessInstance,
    processInstance: ProcessCollection,
}
