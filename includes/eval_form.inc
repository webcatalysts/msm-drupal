<?php


function msm_eval_form($form, &$form_state, array $collection) {
  $source = msm_request(sprintf('collection/%s', $collection['source']));
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
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
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );
  $form['#collection'] = $collection;
  dsm($collection);
  $help = array();
  $help[] = '<h3>Variables:</h3>';
  $help[] = sprintf('<div><strong>source:</strong> %s',
    l($collection['source'], sprintf('admin/structure/msm/col/%s', $collection['_id']))
  );
  $url = 'http://mongodb.github.io/node-mongodb-native/2.2/api/Db.html';
  $help[] = sprintf('<div><strong>%s:</strong> %s</div><div>%s %s</div>',
    'sourceDatabase',
    $source['database'],
    'The source database object.',
    l($url, $url)
  );
  $url = 'http://mongodb.github.io/node-mongodb-native/2.2/api/Collection.html';
  $help[] = sprintf('<div><strong>%s:</strong> %s</div><div>%s %s</div>',
    'sourceCollection',
    $source['collection'],
    'The source collection object.',
    l($url, $url)
  );
  $form['eval']['#prefix'] = implode("\n", $help);
  return $form;
}

function msm_eval_form_submit($form, &$form_state) {
  $collectionId = $form['#collection']['_id'];
  $eval = $form_state['values']['eval'];
  $eval['code'] = $eval['code']['value'];
  $result = msm_post(sprintf('collection/%s/update', $collectionId), array(
    '$set' => array(
      'eval' => $eval,
    ),
  ));
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/schema', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}
