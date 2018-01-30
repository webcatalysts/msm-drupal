var Promise = require('promise');
var BSON = require('bson');
var bson = new BSON();

function projectSchema(project, schema) {
  var removeFields = [];
  var includeFields = [];
  for (var pfield in project) {
    if (project[pfield]) {
      includeFields.push(pfield);
    }
    else {
      removeFields.push(pfield);
    }
  }

  if (removeFields.indexOf('_id') === -1) {
    includeFields.push('_id');
  }
  return projectSchemaRecurse(schema, includeFields, removeFields);
}

function projectSchemaRecurse(schema, includeFields, removeFields, ns) {
  var result = {};
  for (var field_name in schema) {
    var cns = ns ? ns + '.' + field_name : field_name;
    if (removeFields.indexOf(cns) !== -1) continue;
    var field_info = schema[field_name];

    if (includeFields.indexOf(cns) !== -1) {
      result[field_name] = field_info;
    }
    else if (field_info.children) {
      var children = projectSchemaRecurse(field_info.children, includeFields, removeFields, cns);
      if (Object.keys(children).length) {
        field_info.children = children;
        result[field_name] = field_info;
      }
    }
  }
  return result;
}

function fieldIsIncludedInProject(project, field_name) {
  if (typeof project[field_name] !== 'undefined') {
    return project[field_name];
  }
  var removeFields = [];
  var includeFields = [];
  for (var pfield in project) {
    if (project[pfield]) {
      includeFields.push(pfield);
    }
    else removeFields.push(pfield);
  }

  var numR = removeFields.length;
  var numI = includeFields.length;
  for (var r = 0; r < numR; r++) {
  }
  for (var i = 0; i < numI; i++) {
  }
  return true;
}

function flattenSchemaFields(schema, ns) {
  var result = {};
  for (var f in schema) {
    var v = schema[f];
    if (ns) {
      v.parent = ns;
      var cns = ns + '.' + f;
      v.parents = cns.split('.');
    }
    else {
      var cns = f;
      v.parents = [f];
    }
    if (v.type == 'group' || v.type == 'array') {
      if(v.children && Object.keys(v.children).length) {
        var ch = v.children;
        delete v.children;
        result = Object.assign({}, flattenSchemaFields(ch, cns), result);
      }
    }
    result[cns] = v;
  } 
  return result;
}

function flattenResults(results) {
    var ret = [];
    results.forEach(function (result) {
        ret.push(flattenResult(result));
    });
    return ret;
}
function flattenResult(result, ns = null) {
    console.log('flatten');
    var ret = {}
    if (typeof result === 'object') {
        Object.keys(result).forEach(function(key) {
            var cns = ns ? ns + '.' + key : key;
            var value = result[key];
            if (value instanceof Array) {
                ret[cns] = value;
            }
            else if (value instanceof Object) {
                var subret = flattenResult(value, cns);
                ret = Object.assign({}, ret, subret);
            }
            else {
                ret[cns] = value;
            }
        });
    }
    return ret;
}

function expandSchema(fields) {
    var fieldNames = Object.keys(fields).sort();
    var numFields = fieldNames.length;
    var schema = {};
    for (var f = 0; f < numFields; f++) {
        var fieldName = fieldNames[f];
        var field = fields[fieldName];
        var pstr = field.parents.join('.children.');
        var parents = pstr.split('.');
        schema = setNestedValue(schema, parents, field);
    }
    return schema;
}

function mergeSchema(a,b) {
    var af = flattenSchemaFields(a);
    var bf = flattenSchemaFields(b);
    var afn = Object.keys(af);
    var bfn = Object.keys(bf);
    var allFields = afn.concat(bfn).filter(function(value, index, self) {
        return self.indexOf(value) === index;
    }).sort(function(a,b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    });
    var nF = allFields.length;
    var res = {};
    for (var i = 0; i < nF; i++) {
        var f = allFields[i];
        var afv = afn.indexOf(f) !== -1 ? af[f] : false;
        var bfv = bfn.indexOf(f) !== -1 ? bf[f] : false;
        if (afv && bfv) {
            var v = Object.assign({}, bfv, afv);
        }
        else if (afv) {
            var v = afv;
        }
        else if (bfv) {
            var v = bfv;
        }
        else {
            continue;
        }
        res[f] = v;
    }
    return expandSchema(res);
}

function setNestedValue(obj, parents, value, force = false) {
    var schema = obj;
    var len = parents.length;
    for (var i = 0; i < len-1; i++) {
        var elem = parents[i];
        if (!schema[elem]) schema[elem] = {};
        schema = schema[elem];
    }
    schema[parents[len-1]] = value;
    return obj;

    var ref = object;
    var numP = parents.length;
    for(var i = 0; i < numP-1; i++) {
        var p = parents[i];
        if (typeof ref === 'undefined') {
            ref = {};
        }
        if (typeof ref[p] === 'undefined') {
            ref[p] = {};
        }
        ref = ref[p];
    }
    ref = value;
}

var analyzeSchema = function (con, dbName, colName, options) {
    console.log('Analyzing schema for ' + dbName + '.' + colName + '.');

    var schemaDatabase = 'varietyResults';
    var schemaCollection = dbName + '.' + colName + '.schemaFields';
    options = Object.assign({
        limit: 100,
        persistResults: true,
        resultsCollection: schemaCollection,
    }, options);
    return new Promise(function (fulfill, reject) {
        schemaAnalysisBuildCode(colName, options)
            .then(function (code) {
                var db = con.db(dbName);
                db.eval(code, [], {}, function (err, result) {
                    if (err) reject(err);
                    else {
                        fulfill({"dbName": schemaDatabase, "colName": schemaCollection});
                    }
                });
            })
            .catch(reject);
    });
}

var schemaAnalysisBuildCode = function (colName, options) {
    console.log('Building code..');
    var fs = require('fs');
    //var utils = require('./node_modules/variety-cli/lib/utils');
    var utils = require('./utils');
    var libpath = './node_modules/variety/variety.js';
    return new Promise(function (fulfill, reject) {
        fs.readFile(libpath, {}, function (err, data) {
            if (err) {
                console.log('Error: %s', err);
                reject(err);
            }
            else {
                var code = 'this.collection = "' + colName + '";'
                         + 'this.persistResults = "true";'
                         + 'this.resultsCollection = "' + options.resultsCollection + '";'
                         + 'var __quiet = true;'
                         + data;
                var code = utils.buildVarietyParams(colName, options) + data;
                code = JSON.stringify('function () { ' + code + '};');
                fulfill(eval(code));
            }
        });
    });
}

var extractSchema = function (con, dbName, colName) {
    console.log('Extracting schema for: ' + dbName + '.' + colName + '.');
    return new Promise(function (fulfill, reject) {
        con.db(dbName).collection(colName).find().toArray(function (err, fields) {
            if (err) reject(err);
            else fulfill(buildSchemaFromFields(fields));
        });
    });
}

var buildSchemaFromFields = function(fields) {
    var fieldNames = [];
    var schema = {};
    var numFields = fields.length;
    for (var i = 0; i < numFields; i++) {
        var field = fields[i];
        var value = {};
        if (typeof field.value.types.Number !== 'undefined' || typeof field.value.types.NumberLong !== 'undefined') {
            value.type = 'int';
        }
        else if (typeof field.value.types.String !== 'undefined') {
            value.type = 'text';
        }
        else if (typeof field.value.types.Object !== 'undefined') {
            value.type = 'group';
            value.children = {};
        }
        else if (typeof field.value.types.Array !== 'undefined') {
            value.type = 'array';
            value.children = {};
        }
        else if (typeof field.value.types.Boolean !== 'undefined') {
            value.type = 'boolean';
        }
        else if (typeof field.value.types.Date !== 'undefined') {
            value.type = 'date';
        }
        else if (typeof field.value.types.ObjectId !== 'undefined') {
            value.type = 'id';
        }
        else {
            console.log('Unextracted field:');
            console.log(field);
            continue;
        }

        value.totalOccurrences = field.totalOccurrences;
        value.percentContaining = field.percentContaining;

        var fieldName = field['_id'].key;

        if (fieldName.match(/^_versionHash/) || fieldName.match(/^_changeHistory/)) {
            continue;
        }

        if (fieldName.match(new RegExp(/XX\./))) {
            var fieldName = fieldName.replace(/XX\./g, "");
        }
        if (fieldName.match(/\./)) {
            var parts = fieldName.split('.');
            var lastPart = parts.length-1;
            var fieldName = parts[lastPart];
            delete parts[lastPart];
            var parts = parts.filter(function (part) { return typeof part !== 'undefined' });
            var parentFieldName = parts.length > 1 ? parts.join('.children.') : parts[0];
            eval('var fieldName = "' + parentFieldName + '.children.' + fieldName + '";');
        }
        fieldNames.push(fieldName);
        eval('schema.' + fieldName + ' = value;');
    }
    return schema;
}

var sortSchema = function (schemaIn) {
    var schemaOut = {};
    var fields = [];
    var fieldNames = Object.keys(schemaIn);
    var numFields = fieldNames.length;
    for (var i = 0; i < numFields; i++) {
        var f = fieldNames[i];
        var field = schemaIn[f];
        field.display = field.display && Object.keys(field.display).length ? field.display : {};
        field.display.weight = typeof field.display.weight === 'undefined' ? 0 : field.display.weight;
        field.__fieldName = f;
        if (field.children && Object.keys(field.children).length) {
            field.children = sortSchema(schemaIn[f].children);
        }
        fields.push(field);
    }
    fields.sort(function(a,b) {
        if (a.display.weight < b.display.weight) return -1;
        if (a.display.weight > b.display.weight) return 1;
        return 0;
    });
    for (var i = 0; i < numFields; i++) {
        var f = fields[i].__fieldName;
        delete fields[i].__fieldName;
        schemaOut[f] = fields[i];
    }
    return schemaOut;
}

module.exports = {
    flattenSchemaFields: flattenSchemaFields,
    flattenResults: flattenResults,
    projectSchema: projectSchema,
    analyzeSchema: analyzeSchema,
    extractSchema: extractSchema,
    mergeSchema: mergeSchema,
    expandSchema: expandSchema,
    sortSchema: sortSchema,
}
