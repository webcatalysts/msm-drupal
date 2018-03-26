(function ($) {
  Drupal.behaviors.msmCollectionSchemaGrid = {
    attach: function (context, settings) {
      this.init(context, settings.msm_collection_schema_grid);
    },
    refresh: function () {
      var expanded = [];
      this.gridOptions.api.forEachNode(function (node) {
        if (node.expanded)  {
          expanded.push(node.data.id);
        }
      });
      this.gridOptions.api.setRowData(this.immutableStore);
      expanded.forEach(function (gid) {
        this.gridOptions.api.getRowNode(gid).setExpanded(true);
      }.bind(this));
    },
    setSchema: function (schema) {
      this.schemaStore = schema;
      this.immutableStore = this.processSchema(schema);
      return this;
    },
    setImmutableStore: function (data) {
      this.immutableStore = data;
      this.schemaStore = MongoSchemaManager.expandSchema(data);
      return this;
    },
    init: function (context, settings) {
      this.gridIsReady = false;
      this.settings = Object.assign({
        weight_min: -25,
        weight_max: 25,
      }, settings);
      agGrid.LicenseManager.setLicenseKey(this.settings.license_key);
      this.currentContext = context;
      this.gridOptions = {
        components: Object.assign({}, {
          fieldCellRenderer: this.getFieldCellRenderer(),
        }, Drupal.agGrid.components),
        columnDefs: [
          { field: 'id', hide: true },
          {
            headerName: Drupal.t('Title'),
            field: 'display.title',
            editable: true,
            width: 150,
          },
          { headerName: Drupal.t('Abbreviation'),
            field: 'display.abbreviation', editable: true, width: 80 },
          { headerName: Drupal.t('Description'),
            field: 'display.description', editable: true, width: 100, cellEditor: 'agLargeTextCellEditor' },
          { headerName: Drupal.t('Formatter'),
            field: 'display.formatter', editable: true, },
          { headerName: Drupal.t('Priority'),
            field: 'display.priority', editable: true, width: 100 },
          { headerName: Drupal.t('Weight'), field: 'display.weight',
            editable: true,
            width: 100,
            type: 'numericColumn',
            cellEditor: 'agSelectCellEditor',
            cellEditorParams: { values: this.getWeightOptions() }
          },
          { headerName: Drupal.t('Hidden'),
            field: 'display.hidden',
            editable: true,
            width: 90,
            valueFormatter: Drupal.agGrid.valueFormatters.getBoolean(Drupal.t('Hidden'), Drupal.t('Visible')),
            //valueFormatter: Drupal.agGrid.valueFormatters.getBoolean(Drupal.t('Hidden'), Drupal.t('Visible')),
            cellEditor: 'agSelectCellEditor',
            cellEditorParams: { values: [true, false] }
          },
          { headerName: Drupal.t('Visible'),
            field: 'display.visible', editable: true, width: 50 },
          { field: 'filterable', editable: true, width: 50 },
        ],
        enableSorting: false,
        enableFilter: false,
        enableColResize: true,
        treeData: true,
        animateRows: true,
        singleClickEdit: true,
        enableCellChangeFlash: true,
        stopEditingWhenGridLosesFocus: true,
        groupDefaultExpanded: 0,
        //groupDefaultExpanded: false,
        getDataPath: function (data) {
          return data.parents;
        },
        getRowNodeId: function (data) {
          return data.id;
        },
        defaultColDef: {
          suppressMenu: true,
          suppressSorting: true,
        },
        autoGroupColumnDef: {
          rowDrag: true,
          headerName: 'Fields',
          lockPosition: true,
          lockVisible: true,
          width: 300,
          checkboxSelection: true,
          //suppressNavigable: true,
          cellRendererParams: {
            suppressCount: false,
            innerRenderer: 'fieldCellRenderer',
          }
        },
        onRowDragEnd: this.onRowDragEnd.bind(this),
        onRowDragMove: this.onRowDragMove.bind(this),
      };
      this.gridDiv = $('#' + this.settings.html_id, context).get(0);
      if (!this.gridDiv) {
        throw new Error('Grid div not found for: #' + this.settings.html_id);
      }
      new agGrid.Grid(this.gridDiv, this.gridOptions);
      this.setSchema(MongoSchemaManager.sortSchema(this.settings.schema));
      this.refresh();
    },
    processSchema: function (schema, ns = null, rowData = null) {
      if (rowData === null) rowData = [];
      Object.keys(schema).forEach(function (fieldKey, idx) {
        var cns = ns ? ns + '.' + fieldKey : fieldKey;
        var schemaField = Object.assign({
          display: {},
        }, schema[fieldKey]);
        schemaField.display = Object.assign({
          weight: idx
        }, schemaField.display);
        schemaField.display.weight = idx;
        rowData.push({
          id: cns,
          parentId: ns,
          parents: cns.split('.'),
          display: schemaField.display,
        });
        if (schemaField.children && Object.keys(schemaField.children).length) {
          this.processSchema(schemaField.children, cns, rowData);
        }
      }.bind(this));
      return rowData;
    },
    getFieldCellRenderer: function () {
      function FieldCellRenderer() {}
      FieldCellRenderer.prototype.init = function (params) {
        var tempDiv = document.createElement('div');
        var value = params.value;
        tempDiv.innerHTML = '<span>' + value + '</span>';
        this.eGui = tempDiv.firstChild;
      };
      FieldCellRenderer.prototype.getGui = function () {
        return this.eGui;
      };
      return FieldCellRenderer;
    },
    getWeightOptions: function () {
      var weights = [];
      for (var i = this.settings.weight_min; i <= this.settings.weight_max; i++) {
        weights.push(i);
      }
      return weights;
    },
    saveSchema: function () {
      var res = {};
      this.gridOptions.api.forEachNode(function (rowNode) {
        var id = rowNode.data.id;
        res[id] = {
          parents: rowNode.data.parents,
          display: rowNode.data.display
        };
      });
      this.setImmutableStore(res);
      $.ajax({
        type: 'POST',
        url: this.settings.save_url,
        data: this.schemaStore,
        dataType: 'json',
        success: function () {
          alert('Saved!');
        },
      });
    },
    deleteRows: function () {
      console.log(this.gridOptions.api.getSelectedNodes());
    },
    expandAll: function (state = true) {
      this.gridOptions.api.forEachNode(function (rowNode) {
        rowNode.setExpanded(state);
      });
    },
    onRowDragMove: function (event) {
      let movingNode = event.node;
      let overNode = event.overNode;
      if (movingNode.data.id == overNode.data.id || movingNode.data.parentId != overNode.data.parentId) return;
      console.log(movingNode.data);
      var fromIndex = this.immutableStore.indexOf(movingNode.data);
      var toIndex = this.immutableStore.indexOf(overNode.data);
      console.log('From index: ' + fromIndex);
      console.log('To index: ' + toIndex);
      var newStore = this.immutableStore.slice();
      this.moveInArray(newStore, fromIndex, toIndex);
      this.setImmutableStore(newStore);
      this.refresh();
      this.gridOptions.api.clearFocusedCell();
    },
    onRowDragEnd: function (event) {
      var counts = {};
      this.immutableStore.forEach(function (v, idx) {
        if (typeof counts[v.parentId] === 'undefined') {
          counts[v.parentId] = 0;
        }
        counts[v.parentId]++;
        v.display.weight = counts[v.parentId];
      });
      this.refresh();
      this.gridOptions.api.clearFocusedCell();
    },
    moveInArray: function (arr, fromIndex, toIndex) {
      var element = arr[fromIndex];
      arr.splice(fromIndex, 1);
      arr.splice(toIndex, 0, element);
    },
  };
  function valueFormatterBooleanYesNo(params) {
    switch (params.value) {
      case true: return Drupal.t('Yes');
      case false: return Drupal.t('No');
      default: return params.value ? params.value : '-';
    }
  }
})(jQuery);
