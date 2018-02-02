
class ImportSource {
    constructor (options = {}) {
        this.options = options;
    }
    async get (path, params = {}) {
        this.mergeParams(params);
        await this.preLoad(path, params);
        let results = await this.load(path, params);
        await this.postLoad(results, path, params);
        return results;
    }
    async preLoad (path, params = {}) {}
    async postLoad(results, path, params = {}) {
        this.prepareResultData(results);
    }
    getDefaultParams () {
        return {};
    }
    prepareResultData (data) {
        return data;
    }
    mergeParams(params = {}) {
        params = Object.assign({},this.getDefaultParams(), params);
        return params;
    }
}

module.exports = ImportSource;
