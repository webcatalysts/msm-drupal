SettingsProvider = function (databaseWrapper, collectionName = 'msm_settings', databaseName = 'msm') {
    this.collectionName = collectionName;
    this.databaseName = databaseName;
    this.databaseWrapper = databaseWrapper;
}

SettingsProvider.prototype.getCollection = async function () {
    await this.databaseWrapper.connect();
    var collection = this.databaseWrapper
        .connection
        .db(this.databaseName)
        .collection(this.collectionName);
    return collection;
}
SettingsProvider.prototype.saveMany = async function (settings) {
    var keys = Object.keys(settings);
    var numKeys = keys.length;
    var collection = await this.getCollection();
    for (var i in settings) {
        await collection.updateOne({_id: i},
            { value: settings[i] },
            { bypassDocumentValidation: true, upsert: true }
        );
    }
    this.databaseWrapper.close();
    return true;
}



SettingsProvider.prototype.load = async function (query = {}) {
    let collection = await this.getCollection();
    let docs = await collection.find(query).toArray();
    this.databaseWrapper.close();
    var settings = {};
    var numDocs = docs.length;
    for (var i = 0; i < numDocs; i++) {
        var doc = docs[i];
        settings[doc._id] = doc.value;
    }
    return settings;
}

exports.SettingsProvider = SettingsProvider;
