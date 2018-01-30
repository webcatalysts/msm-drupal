<?php

function msm_mapreduce_form($form, &$form_state, array $collection) {
  $mapReduce = empty($form_state['mapReduce']) ? array() : $form_state['mapReduce'];
  $finalize = empty($form_state['finalize']) ? array() : $form_state['finalize'];
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );

  $form_state['mapReduce'] = $mapReduce;
  $form_state['finalize'] = $finalize;
  $form['#collection'] = $collection;
  return $form;
}

function msm_mapreduce_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
}

function msm_mapreduce_form_submit($form, &$form_state) {
  $collectionId = $form['#collection']['_id'];
  $result = msm_post(sprintf('collection/%s/update', $collectionId), array(
    '$set' => array(
      'mapReduce' => array(
        'mapReduce' => $form_state['mapReduce'],
        'finalize' => $form_state['finalize'],
      ),
    ),
  ));
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s/schema', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}
