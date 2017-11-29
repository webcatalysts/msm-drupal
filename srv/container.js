function loadContainer(db, containerId) {
    return db.collection('containers').findOne({_id: containerId});
}

function loadCollection(db, collectionId) {
    return db.collection('collections').findOne({_id: containerId});
}

exports.query = function (db, res, containerId) {
    return 'hi';
    var container = loadContainer(containerId);
    return container;
    var collection = loadCollection(container.collection);
    return collection;
}
