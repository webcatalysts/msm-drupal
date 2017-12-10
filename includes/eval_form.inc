<?php


function msm_eval_form($form, &$form_state, array $collection) {
  $source = msm_request(sprintf('collection/%s', $collection['source']));
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );
  if ($collection['persist']) {
    $form['actions']['save_and_process'] = array(
      '#type' => 'submit',
      '#value' => t('Save and Process'),
    );
  }
  $form['eval'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Custom evaluation'),
  );
  $form['eval']['code'] = array(
    '#title' => t('Code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#required' => TRUE,
    '#rows' => 20,
    '#default_value' => !empty($collection['eval']['code']) ? $collection['eval']['code'] : null,
  );
  $form['#collection'] = $collection;

  $help = array();
  $help[] = array(
    'label' => 'callback',
    'value' => 'function',
  );
  $help[] = array(
    'label' => 'source',
    'value' => l($source['collection'], sprintf('admin/structure/msm/col/%s', $collection['_id'])),
    'description' => t('The source collection object.'),
  );
  $help[] = array(
    'label' => 'sourceDatabase',
    'value' => l($source['database'], sprintf('admin/structure/msm/db/%s', $source['database'])),
    'description' => t('The source database object.'),
    'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Db.html',
  );
  $help[] = array(
    'label' => 'sourceDatabaseName',
    'value' => $source['database'],
    'The source database name (string).',
  );
  $help[] = array(
    'label' => 'sourceCollection',
    'value' => $source['collection'],
    'description' => t('The source collection object.'),
    'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Collection.html',
  );
  $help[] = array(
    'label' => 'sourceCollectionName',
    'value' => $source['collection'],
    'description' => t('The source collection name (string).'),
  );
  if ($collection['persist']) {
    $help[] = array(
      'label' => 'destinationDatabase',
      'value' => l($collection['database'], sprintf('admin/structure/msm/db/%s', $collection['database'])),
      'description' => t('The destination database object.'),
      'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Db.html',
    );
    $help[] = array(
      'label' => 'destinationDatabaseName',
      'value' => $collection['database'],
      'description' => t('The destination database name (string).'),
    );
    $help[] = array(
      'label' => 'destinationCollection',
      'value' => $collection['collection'],
      'description' => t('The destination collection object.'),
      'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Collection.html',
    );
    $help[] = array(
      'label' => 'destinationCollectionName',
      'value' => $collection['collection'],
      'description' => t('The destination collection name (string).'),
    );
  }
  $form['help'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Help'),
    'variables' => array(
      '#markup' => theme('msm_variables_help', array('title' => t('Variables'), 'items' => $help)),
    ),
    '#weight' => -10,
  );
  return $form;
}

function msm_eval_form_submit($form, &$form_state) {
  $collectionId = $form['#collection']['_id'];
  $eval = $form_state['values']['eval'];
  $eval['code'] = $eval['code']['value'];
  $ele = $form_state['triggering_element'];
  $path = $ele['#value'] == t('Save and Process') ? 'collection/%s/process' : 'collection/%s/update';
  $result = msm_post(sprintf($path, $collectionId), array(
    '$set' => array(
      'eval' => $eval,
    ),
  ));
  dsm($result);

  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/schema', $collectionId);
  }
  else form_set_error('', t('An error has occured while attempting to update the collection: @error', array('@error' => $result['error'])));
}
