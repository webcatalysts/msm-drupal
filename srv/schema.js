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
      if(v.children && v.children.length) {
        result = Object.assign(result, flattenSchemaFields(v.children, cns));
        delete v.children;
      }
    }
    result[cns] = v;
  }
  return result;
}

function expandSchema(fields) {
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
                console.log(err);
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
        if (typeof field.value.types.Number !== 'undefined' || typeof field.value.types.NumberLong !== 'undefined') {
            var value = { type: 'int' };
        }
        else if (typeof field.value.types.String !== 'undefined') {
            var value = { type: 'text' };
        }
        else if (typeof field.value.types.Object !== 'undefined') {
            var value = { type: 'group', children: {} };
        }
        else if (typeof field.value.types.Array !== 'undefined') {
            var value = { type: 'array', children: {} };
        }
        else if (typeof field.value.types.Boolean !== 'undefined') {
            var value = { type: 'boolean' };
        }
        else { continue; }

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

module.exports = {
    flattenSchemaFields: flattenSchemaFields,
    projectSchema: projectSchema,
    analyzeSchema: analyzeSchema,
    extractSchema: extractSchema,
}
