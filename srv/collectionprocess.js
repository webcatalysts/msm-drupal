

var CollectionProcessInstance = function (collection, databaseWrapper, databaseProvider, collectionProvider) {
    this.collection = collection;
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.collectionProvider = collectionProvider;
}

var ProcessCollection = function (instance, callback) {
    if (instance.collection.type) {
    }
    else if (instance.collection.schemaImported) {
        callback(null, { ok: 0 });
    }
}

module.exports = {
    createInstance: CollectionProcessInstance,
    processInstance: ProcessCollection,
}
