
class ImportSource {
    constructor (options = {}) {
        this.options = options;
    }
    get (path, params = {}) {
        var results = this.load(path, params);
        this.prepareResultData(results);
        return results;
    }
    getDefaultParams () {
        return {};
    }
    prepareResultData (data) {
        return data;
    }
}

module.exports = ImportSource;
