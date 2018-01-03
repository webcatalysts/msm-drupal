<?php

function msm_collection_list() {
  $collections = msm_post('collections', [
    'project' => ['name' => 1, 'database' => 1, 'collection' => 1, 'type' => 1,
      'enabled' => 1, 'source' => 1, 'dependencies' => 1,
    ],
    'sort' => ['weight' => 1, 'name' => 1, '_id' => 1],
  ]);
  dsm($collections);
  $build = array();
  $build['list'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Name'),
      t('Database'),
      t('Collection'),
      t('Type'),
      t('Status'),
      t('Source Collection'),
      t('Dependencies'),
      t('Actions'),
    ),
    '#rows' => array(),
  );
  foreach ($collections AS $col) {
    $actions = array(
      '#theme' => 'links',
      '#links' => array(),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    if (msm_collection_access($col, 'endpoint')) {
      $actions['#links']['endpoint'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/endpoint', $col['_id']),
        'title' => t('Endpoint'),
      );
    }
    if (msm_collection_access($col, 'settings')) {
      $actions['#links']['settings'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/settings', $col['_id']),
        'title' => t('Settings'),
      );
    }
    if (msm_collection_access($col, 'schema')) {
      $actions['#links']['schema'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/schema', $col['_id']),
        'title' => t('Schema'),
      );
    }
    if (msm_collection_access($col, 'eval')) {
      $actions['#links']['eval'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/custom', $col['_id']),
        'title' => t('Eval'),
      );
    }
    elseif (msm_collection_access($col, 'pipeline')) {
      $actions['#links']['pipeline'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/pipeline', $col['_id']),
        'title' => t('Pipeline'),
      );
    }
    elseif (msm_collection_access($col, 'mapReduce')) {
      $actions['#links']['map-reduce'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/map-reduce', $col['_id']),
        'title' => t('Map Reduce'),
      );
    }
    if (msm_collection_access($col, 'process')) {
      $actions['#links']['process'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/process', $col['_id']),
        'title' => t('Process'),
      );
    }
    $actions['#links']['delete'] = array(
      'href' => sprintf('admin/structure/msm/col/%s/delete', $col['_id']),
      'title' => t('Delete'),
    );
    $dependencies = sprintf('<em>%s</em>', t('None'));
    if (!empty($col['dependencies'])) {
      $dependencies = array(
        '#theme' => 'links',
        '#links' => array(),
      );
      foreach ($col['dependencies'] AS $dep) {
        $dependencies['#links'][] = array(
          'title' => $dep,
          'href' => sprintf('admin/structure/msm/col/%s', $dep),
        );
      }
      $dependencies = render($dependencies);
    }
    $source = !empty($col['source']) ? l($col['source'], 'admin/structure/msm/col/%s') : 'n/a';
    $build['list']['#rows'][] = array(
      l($col['name'], sprintf('admin/structure/msm/col/%s', $col['_id'])),
      l($col['database'], sprintf('admin/structure/msm/db/%s', $col['database'])),
      $col['collection'],
      empty($col['type']) ? 'n/a' : $col['type'],
      $col['enabled'] ? t('Enabled') : t('Disabled'),
      $source,
      $dependencies,
      render($actions),
    );
  }
  return $build;
}

function msm_collection_view(array $collection, array $options = array()) {
  $options += array(
    'info' => TRUE,
    'schema_info' => TRUE,
    'schema' => TRUE,
  );
  $module_path = drupal_get_path('module', 'msm');
  $build = array(
    '#theme' => 'msm_collection_view',
    '#attached' => array(
      'css' => array($module_path . '/css/collection-view.css'),
    ),
  );
  $has_schema = !empty($collection['schema']);
  if ($options['info']) {
    $count = msm_request(sprintf('collection/%s/count', $collection['_id']));
    if ($count['ok']) {
        $count = $count['count'];
    }
    else $count = 'err';

    $build['#info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Collection details'),
      '#items' => array(
        '_id' => array(t('Collection ID'), $collection['_id']),
        'name' => array(t('Name/Label'), $collection['name']),
        'collection' => array(t('Collection Name'), $collection['collection']),
        'database' => array(t('Database'), l($collection['database'], 'admin/structure/msm/db/' . $collection['collection'])),
        'type' => array(t('Collection Type'), $collection['type']),
        'schema' => array(t('Has Schema'), $has_schema ? t('Yes') : t('No')),
        'totalDocuments' => array(t('Total Documents'), number_format($count, 0)),
      ),
    );
    if (!empty($collection['source'])) {
      $build['#info']['items'][] = array(t('Source Collection ID'), l($collection['source'], 'admin/structure/msm/col/' . $collection['source']));
    }
  }
  if ($options['schema_info'] && $has_schema) {
    $fields = msm_schema_flatten_fields($collection['schema'], MSM_SCHEMA_FLATTEN_ALL);
    $build['#schema_info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Schema breakdown'),
      '#items' => array(
        'total_fields' => array(t('Total fields'), count($fields)),
      ),
    );
    $unused_fields = array();
    $counts = [];
    foreach ($fields AS $field_name => $field) {
      if (isset($field['percentContaining']) && $field['percentContaining'] <= 0) {
        $unused_fields[] = $field_name;
      }
      $type = $field['type'];
      if (!isset($counts[$type])) $counts[$type] = 0;
      $counts[$type]++;
    }
    foreach ($counts AS $type => $count) {
      $build['#schema_info']['#items'][] = array(ucfirst($type), $count);
    }
    if ($unused_fields) {
      $build['#schema_info']['#items'][] = array(t('Unused fields'), theme('item_list', array('items' => $unused_fields)));
    }
  }
  if ($options['schema']) {
    if (!empty($collection['schema'])) {
      $build['#schema'] = array(
        '#prefix' => sprintf('<h3>%s</h3>', t('Schema')),
        '#theme' => 'msm_schema_table',
        '#schema' => msm_schema_sort($collection['schema']),
        '#flatten' => TRUE,
      );
    }
    else {
      $build['#schema'] = array(
        '#markup' => t('No schema exists yet.'),
      );
    }
  }
  return $build;
}

function msm_collection_schema_view(array $collection) {
  if (!empty($collection['schema'])) {
    $build['schema'] = array(
      '#theme' => 'msm_schema_table',
      '#schema' => $collection['schema'],
      '#flatten' => TRUE,
    );
  }
  else {
    $build['schema'] = array(
      '#markup' => t('No schema exists yet.'),
    );
  }
  return $build;
}

function msm_collection_create_form($form, &$form_state) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $ajax = array(
    'callback' => 'msm_collection_create_form_ajax',
    'wrapper' => $html_id,
  );
  $document = !empty($form_state['document']) ? $form_state['document'] : array();
  $document += array(
    'enabled' => FALSE,
    'persist' => null,
    'type' => null,
    'source' => null,
  );
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#access' => !empty($document['source']),
    '#required' => TRUE,
    '#weight' => 95,
    '#ajax' => $ajax + array('event' => 'blur'),
  );
  $form['machine'] = array(
    '#title' => t('Machine'),
    '#type' => 'machine_name',
    '#machine_name' => array(
      'exists' => 'msm_collection_exists',
    ),
    '#access' => !empty($document['source']),
    '#disabled' => $document['persist'],
    '#required' => TRUE,
    '#weight' => 97,
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create collection'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/structure/msm/col',
    '#title' => t('Cancel'),
  );
  $form['persist'] = array(
    '#title' => t('Storage type'),
    '#type' => 'radios',
    '#options' => array(
      1 => t('Normal (persist)'),
      0 => t('Real-time Query Container (no storage)'),
    ),
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['type'] = array(
    '#title' => t('Collection type'),
    '#type' => 'select',
    '#options' => msm_collection_type_options(),
    '#access' => $document['persist'] !== null,
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['source'] = array(
    '#title' => t('Source collection'),
    '#type' => 'select',
    '#options' => array('') + msm_collection_options(),
    '#access' => !empty($document['type']),
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  if (!empty($document['source'])) {
    if (empty($form['#source']) || $form['#source']['_id'] != $document['source']) {
      $form['#source'] = msm_request(sprintf('collection/%s', $document['source']));
    }
  }
  if ($document['persist'] && !empty($document['source'])) {
    $document += array('destination' => array());
    $document['destination'] += array(
      'database' => $form['#source']['database'],
      'collection' => NULL,
    );
    $form['destination'] = array(
      '#tree' => TRUE,
      '#access' => !empty($document['source']) && $document['persist'],
    );
    if ($document['type'] == 'mapReduce') {
      $form['destination']['database'] = array(
        '#title' => t('Destination database'),
        '#type' => 'select',
        '#options' => msm_database_options(),
        '#default_value' => $form['#source']['database'],
        '#required' => TRUE,
      );
    }
    else {
        $form['destination']['database'] = array(
          '#type' => 'value',
          '#value' => $document['destination']['database'],
        );
    }
    $form['destination']['collection'] = array(
      '#title' => t('Destination collection'),
      '#type' => 'textfield',
      '#description' => t('Specify the name of the collection to create.'),
      '#required' => TRUE,
      '#ajax' => $ajax += array('event' => 'blur'),
    );
  }
  if (empty($form_state['id'])) {
    $form['actions']['submit']['#access'] = FALSE;
  }

  $form_state['document'] = $document;
  return $form;
}

function msm_collection_create_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
  $document =& $form_state['document'];
  $skip = array('form_id', 'form_build_id', 'form_token', 'machine', 'op', 'submit', 'destination');
  foreach (array_keys($values) AS $key) {
    if (in_array($key, $skip)) continue;
    $document[$key] = $values[$key];
  }
  $document['persist'] = intval($document['persist']) ? true : false;
  if (!$document['persist']) {
    $form_state['id'] = empty($values['machine']) ? null : $values['machine'];
    $document['collection'] = $form['#source']['collection'];
    $document['database'] = $form['#source']['database'];
  }
  else {
    if (!empty($values['destination']['collection']) && !empty($values['destination']['database'])) {
      $document['collection'] = $values['destination']['collection'];
      $document['database'] = $values['destination']['database'];
      $form_state['id'] = sprintf('%s.%s', $document['database'], $document['collection']);
    }
    else $form_state['id'] = null;
  }
}

function msm_collection_create_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_create_form_submit($form, &$form_state) {
  $result = msm_post(sprintf('collection/create/%s', $form_state['id']), $form_state['document']);
  if ($result['ok']) {
    switch ($form_state['document']['type']) {
      case 'mapReduce';
        $step = 'map-reduce';
        break;
      case 'aggregation';
        $step = 'pipeline';
        break;
      case 'custom':
        $step = 'custom';
        break;
    }
    $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/%s', $form_state['id'], $step);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_delete_form($form, &$form_state, $collection) {
  $form['#collection'] = $collection;
  return confirm_form($form,
    t('Are you sure you want to delete all schema information for @id', array('@id' => $collection['_id'])),
    sprintf('admin/structure/msm/col/%s', $collection['_id']),
    t('No user database will be deleted. Only schema information.'),
    t('Delete collection'),
    t('Cancel')
  );
}

function msm_collection_delete_form_submit($form, &$form_state) {
  $result = msm_request(sprintf('collection/%s/delete', $form['#collection']['_id']));
  if ($result['ok']) {
    $form_state['redirect'] = 'admin/structure/msm/col';
  }
}

function msm_collection_type_options() {
  return array(
    'mapReduce' => t('Map Reduce'),
    'aggregation' => t('Aggregation'),
    'custom' => t('Custom'),
  );
}

function msm_collection_exists() {
  $args = func_get_args();
  return false;
}

function msm_collection_settings_form($form, &$form_state, array $collection) {
  $collection = json_decode(json_encode($collection), TRUE);
  $collection += array(
    'preProcess' => NULL,
    'postProcess' => NULL,
    'preExecute' => NULL,
    'postExecute' => NULL,
    'type' => 'existing',
    'persist' => FALSE,
    'weight' => 0,
  );
  $weight_options = range(-300, 300);
  $form['weight'] = array(
      '#type' => 'select',
      '#title' => t('Weight'),
      '#required' => TRUE,
      '#options' => array_combine($weight_options, $weight_options),
      '#default_value' => $collection['weight'],
  );
  $form['process'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Processing'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preProcess']) && empty($collection['postProcess']),
    '#access' => $collection['persist'],
  );
  $form['process']['preProcess'] = array(
    '#title' => t('Pre-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $collection['preProcess'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['process']['postProcess'] = array(
    '#title' => t('Post-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postProcess'],
  );
  $form['execute'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Execution'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preExecute']) && empty($collection['postExecute']),
  );
  $form['execute']['preExecute'] = array(
    '#title' => t('Pre-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['preExecute'],
  );
  $form['execute']['postExecute'] = array(
    '#title' => t('Post-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postExecute'],
  );
  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => $collection['enabled'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_settings_form_validate($form, &$form_state) {
}

function msm_collection_settings_form_submit($form, &$form_state) {
  $fields = array('preExecute', 'postExecute', 'preProcess', 'postProcess');
  $values = $form_state['values'];
  $update = array(
    '$set' => array(
      'enabled' => (bool)$values['enabled'],
      'weight' => (int)$values['weight'],
    ),
    '$unset' => array(),
  );
  $collectionId = $form['#collection']['_id'];
  foreach ($fields AS $f) {
    if (empty($values[$f])) {
      $update['$unset'][$f] = "";
    }
    else $update['$set'][$f] = $values[$f];
  }
  if (empty($update['$unset'])) {
    unset($update['$unset']);
  }
  $result = msm_post(sprintf('collection/%s/update', $collectionId), $update);
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/view', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_endpoint(array $collection) {
  return drupal_get_form('msm_collection_endpoint_form', $collection);
}

function msm_collection_endpoint_form($form, &$form_state, array $collection) {
  $params = empty($form_state['values']['params']) ? array() : $form_state['values']['params'];
  $params += array(
    'limit' => 1,
    'skip' => 0,
    'query' => null,
    'project' => null,
    'sort' => null,
  );
  $result = msm_post(sprintf('collection/%s/query', $collection['_id']), $params);
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $ajax = array(
    'callback' => 'msm_collection_endpoint_form_ajax',
    'wrapper' => $html_id,
  );
  $form['params'] = array(
    '#type' => 'fieldset',
    '#title' => t('Parameters'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['params']['query'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Query'),
    '#default_value' => $params['query'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['sort'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Sort'),
    '#default_value' => $params['sort'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['project'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Project'),
    '#default_value' => $params['project'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['skip'] = array(
    '#type' => 'textfield',
    '#title' => t('Skip'),
    '#default_value' => $params['skip'],
  );
  $form['params']['update'] = array(
    '#type' => 'button',
    '#value' => t('Run query'),
    '#ajax' => $ajax,
  );
  if (!empty($result['results'][0])) {
    $form['result'] = array(
      '#markup' => theme('msm_schema_result_tree', array(
        'schema' => $result['schema'],
        'result' => $result['results'][0],
      )),
    );
  }
  else {
    $form['empty'] = array(
      '#markup' => sprintf('<p>%s</p>', t('No results were found that matched your search.')),
    );
  }
  $form['total'] = array(
    '#type' => 'item',
    '#title' => t('Total documents:'),
    '#markup' => $result['totalResults'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('clearfix')),
  );
  if (((int)$params['skip']) < ($result['totalResults']-1)) {
    $form['actions']['next'] = array(
      '#type' => 'button',
      '#value' => t('Next'),
      '#attributes' => array('style' => 'float:right;'),
      '#ajax' => $ajax,
    );
  }
  if ($params['skip']) {
    $form['actions']['prev'] = array(
      '#type' => 'button',
      '#value' => t('Previous'),
      '#ajax' => $ajax,
    );
  }
  return $form;
}

function msm_collection_endpoint_form_validate(&$form, &$form_state) {
  $params = $form_state['values']['params'];
  $input = &$form_state['input']['params'];

  $ele = $form_state['triggering_element'];
  if ($ele['#parents'][0] == 'next') {
    $params['skip'] = ((int)$params['skip'])+1;
    $input['skip'] = $params['skip'];
  }
  elseif ($ele['#parents'][0] == 'prev') {
    $params['skip'] = ((int)$params['skip'])-1;
    $input['skip'] = $params['skip'];
  }
  $result = array();
  foreach ($params AS $k => $v) {
    if ($v && !empty($v) && $k != 'update') {
      $result[$k] = $v;
    }
  }
  foreach (array('project', 'sort', 'query') AS $k) {
    if (!empty($result[$k])) {
      if (!$result[$k] = json_decode($result[$k])) {
        form_set_error("params][$k", t('Invalid JSON detected.'));
      }
    }
  }
  $form_state['values']['params'] = $result;
}

function msm_collection_endpoint_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_process_form($form, &$form_state, array $collection) {
  return $form;
}

function msm_collection_reset_form($form, &$form_state, array $collection) {
  $form['#collection'] = $collection;
  return confirm_form($form,
    t('Are you sure you want to reset the schema for @collection', array('@collection' => $collection['_id'])),
    sprintf('admin/structure/msm/collection/%s', $collection['_id']),
    t('This will remove the schema information.'),
    t('Reset schema'),
    t('Cancel')
  );
}

function msm_collection_reset_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $update = array(
    '$set' => array('enabled' => false),
    '$unset' => array('schema' => '', 'schemaImported' => '', 'analyzingSchema' => ''),
  );
  $result = msm_post(sprintf('collection/%s/update', $collection['_id']), $update);
  if ($result['ok']) {
    drupal_set_message(t('The schema is being reset. Refresh the page momentarily as necessary.'));
    $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/schema/analyze', $collection['_id']);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_schema_analyze_form($form, &$form_state, array $collection) {
  $result = msm_request(sprintf('collection/%s/count', $collection['_id']));
  $form['#attributes'] = array('class' => array('clearfix'));
  if (!$result['ok']) {
    $form['error'] = array(
      '#markup' => t('Failed to retrieve record count from @db -> @collection', array(
        '@db' => $form['#db'],
        '@col' => $form['#cname'],
      )),
    );
  }
  $form['totalRecords'] = array(
    '#type' => 'item',
    '#title' => t('Total records'),
    '#markup' => $result['count'],
  );
  $form['limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit'),
    '#description' => t('Limit schema analysis to # records.'),
    '#default_value' => $result['count'],
  );
  drupal_set_title(t('Analyze and import schema for @collection', array('@collection' => $collection['_id'])));
  $form['actions'] = array(
    '#prefix' => sprintf('<p>%s</p>', t('This will analyze the existing data to derrive the schema structure and import it as an "existing" collection.')),
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Analyze & Import Schema'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => sprintf('admin/structure/msm/col/%s', $collection['_id']),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_schema_analyze_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $options = array('limit' => $form_state['values']['limit']);
  $result = msm_post(sprintf('collection/%s/analyze', $collection['_id']), $options);
  $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/schema/view', $collection['_id']);
  drupal_set_message(t('The schema is being analyzed. Refresh the page momentarily as necessary.'));
}
