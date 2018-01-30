<?php

function msm_pipeline_form($form, &$form_state, array $collection) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';

  if (empty($form_state['values'])) {
    $pipeline = empty($collection['pipeline']) ? array() : $collection['pipeline'];
  }
  else $pipeline = $form_state['pipeline'];

  $ajax = array(
    'callback' => 'msm_pipeline_form_ajax',
    'wrapper' => $html_id,
  );

  $form['pipeline'] = array(
    '#type' => 'fieldset',
    '#title' => t('Pipeline'),
    '#tree' => TRUE,
  );

  $source = msm_request(sprintf('collection/%s', $collection['source']));
  $schema = $source['schema'];

  foreach ($pipeline AS $i => &$stage) {
    $stage['index'] = $i;
    if (empty($stage['schema'])) {
      $stage['schema'] = $i ? $pipeline[$i-1]['schema'] : $schema;
    }
    $form['pipeline'][$i] = msm_pipeline_stage_widget($stage, $collection, $ajax);
    #$form['pipeline'][$i] = array(
    #);
  }

  $form_state['schema'] = $stage['schema'];

  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );
  $form['add_stage'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Add stage'),
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['add_stage']['type'] = array(
    '#type' => 'select',
    '#title' => t('Stage type'),
    '#options' => msm_pipeline_stage_type_options(),
    '#default_value' => 'project',
  );
  $form['add_stage']['button'] = array(
    '#type' => 'button',
    '#value' => t('Add stage'),
    '#ajax' => $ajax,
  );
  $form['#collection'] = $collection;
  $form_state['pipeline'] = $pipeline;
  return $form;
}

function msm_pipeline_form_ajax($form, &$form_state) {
    dsm($form_state);
  return $form;
}

function msm_pipeline_form_validate(&$form, &$form_state) {
  $ele = $form_state['triggering_element'];
  $values = $form_state['values'];
  $form_state['pipeline'] = $values['pipeline'];
  if ($ele['#type'] == 'button' && $ele['#value'] == t('Add stage')) {
    $form_state['pipeline'][] = array('type' => $values['add_stage']['type']);
    unset($form_state['values']['add_stage']);
    unset($form_state['input']['add_stage']);
  }
  elseif ($ele['#type'] == 'checkbox' && $ele['#title'] == t('Remove stage')) {
    $parents = $ele['#parents'];
    array_pop($parents);
    $idx = array_pop($parents);
    $v = &drupal_array_get_nested_value($form_state['values'], $parents);
    unset($form_state['pipeline'][$idx]);
    unset($v[$idx]);
  }
}

function msm_pipeline_form_submit($form, &$form_state) {
  #dsm($form_state);
  $collectionId = $form['#collection']['_id'];
  $result = msm_post(sprintf('collection/%s/update', $collectionId), array(
    '$set' => array(
      'pipeline' => $form_state['pipeline'],
      'schema' => $form_state['schema'],
    ),
  ));
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s/schema', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_pipeline_stage_type_options() {
  return array(
    'project' => '$project',
    'group' => '$group',
    'limit' => '$limit',
    'skip' => '$skip',
    'lookup' => '$lookup',
    'unwind' => '$unwind',
    'match' => '$match',
    'sort' => '$sort',
  );
}

function msm_pipeline_stage_widget($stage, array $collection, array $ajax) {
  $html_id = drupal_html_id(__FUNCTION__);
  $widget = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('#@num $@type', array('@num' => $stage['index']+1, '@type' => $stage['type'])),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#prefix' => sprintf('<div>%s', $html_id),
    '#suffix' => '</div>',
  );
  $stageajax = array(
    'callback' => 'msm_pipeline_stage_widget_ajax',
    'wrapper' => $html_id,
  );
  $widget['schema'] = array(
    '#type' => 'value',
    '#value' => $stage['schema'],
  );
  $widget['type'] = array(
    '#type' => 'value',
    '#value' => $stage['type'],
  );
  $widget['remove'] = array(
    '#type' => 'checkbox',
    '#title' => t('Remove stage'),
    '#ajax' => $ajax,
  );
  $field_options = msm_schema_field_options($stage['schema']);
  switch ($stage['type']) {
    case 'project':
      $stage += array('project' => array());
      $widget['project'] = msm_pipeline_stage_project_widget($stage, $stageajax);
      $widget['#element_validate'][] = 'msm_pipeline_stage_project_validate';
      break;
    case 'group':
      $widget['group'] = msm_pipeline_stage_group_widget($stage, $stageajax);
      break;
    case 'limit':
      $stage += array('limit' => 100);
      $url = 'https://docs.mongodb.com/manual/reference/operator/aggregation/limit/';
      $widget['limit'] = array(
        '#title' => t('Maximum documents'),
        '#type' => 'textfield',
        '#default_value' => $stage['limit'],
        '#description' => l($url, $url),
      );
      break;
    case 'skip':
      $stage += array('skip' => 1);
      $url = 'https://docs.mongodb.com/manual/reference/operator/aggregation/limit/';
      $widget['skip'] = array(
        '#title' => t('Maximum documents to skip'),
        '#type' => 'textfield',
        '#default_value' => $stage['skip'],
        '#description' => l($url, $url),
      );
      break;
    case 'lookup':
      $stage += array('lookup' => array());
      $stage['lookup'] += array(
        'from' => null,
        'localField' => null,
        'foreignField' => null,
        'alias' => null,
      );
      $widget['lookup']['from'] = array(
        '#required' => TRUE,
        '#title' => t('Lookup collection'),
        '#type' => 'select',
        '#options' => msm_pipeline_stage_lookup_collection_options($collection['database']),
        '#default_value' => $stage['lookup']['from'],
        '#ajax' => $stageajax,
      );
      $widget['lookup']['localField'] = array(
        '#required' => TRUE,
        '#title' => t('Local Field'),
        '#type' => 'select',
        '#options' => $field_options,
        '#default_value' => $stage['lookup']['localField'],
      );
      if ($stage['lookup']['from']) {
        $from_collection = msm_request(sprintf('collection/%s', $stage['lookup']['from']));
        $from_field_options = msm_schema_field_options($from_collection['schema']);
        $widget['lookup']['foreignField'] = array(
          '#type' => 'select',
          '#title'=> t('Foreign Field'),
          '#options' => $from_field_options,
          '#default_value' => $stage['lookup']['foreignField'],
          '#required' => TRUE,
          '#access' => !empty($stage['lookup']['from']),
        );
      }
      break;
    case 'unwind':
      $stage += array('unwind' => array());
      $stage['unwind'] += array(
        'path' => NULL,
        'includeArrayIndex' => NULL,
        'preserveNullAndEmptyArrays' => NULL,
      );
      $url = 'https://docs.mongodb.com/manual/reference/operator/aggregation/unwind/';
      $widget['unwind'] = array(
        '#tree' => TRUE,
        '#suffix' => l($url, $url),
      );
      $widget['unwind']['path'] = array(
        '#required' => TRUE,
        '#title' => t('Path'),
        '#type' => 'select',
        #'#options' => msm_schema_field_options($schema, MSM_SCHEMA
        '#options' => $field_options,
        '#default_value' => $stage['unwind']['path'],
      );
      $widget['unwind']['includeArrayIndex'] = array(
        '#type' => 'checkbox',
        '#title' => t('Include Array Index'),
        '#default_value' => $stage['unwind']['includeArrayIndex'],
      );
      $widget['unwind']['preserveNullAndEmptyArrays'] = array(
        '#type' => 'checkbox',
        '#title' => t('Preserve Null and Empty Arrays'),
        '#default_value' => $stage['unwind']['preserveNullAndEmptyArrays'],
      );
      break;
    case 'match':
      $stage += array('match' => "{\n\n}");
      $url = 'https://docs.mongodb.com/manual/reference/operator/aggregation/match/';
      $widget['match'] = array(
        '#type' => 'text_format',
        '#format' => 'catalyst_js',
        '#title' => t('Match'),
        '#default_value' => $stage['match'],
        '#description' => l($url, $url, array('attributes' => array('target' => '_blank'))),
      );
      $widget['#element_validate'][] = 'msm_pipeline_match_validate';
      break;
    case 'sort':
      $stage += array('sort' => "{\n\n}");
      $url = 'https://docs.mongodb.com/manual/reference/operator/aggregation/sort/';
      $widget['sort'] = array(
        '#title' => t('Sort'),
        '#required' => TRUE,
        '#type' => 'text_format',
        '#format' => 'catalyst_js',
        '#default_value' => $stage['sort'],
        '#description' => l($url, $url, array('attributes' => array('target' => '_blank'))),
      );
      $widget['#element_validate'][] = 'msm_pipeline_sort_validate';
      break;
  }
  return $widget;
}

function msm_pipeline_stage_widget_ajax($form, &$form_state) {
  $ele = $form_state['triggering_element'];
  $parents = $ele['#parents'];
  array_pop($parents);
  array_pop($parents);
  $widget = drupal_array_get_nested_value($form, $parents);
  $widget['#collapsed'] = FALSE;
  $idx = array_search('collapsed', $widget['#attributes']['class']);
  unset($widget['#attributes']['class'][$idx]);
  return $widget;
}

function msm_pipeline_stage_lookup_collection_options($database) {
  $result = msm_request(sprintf('collections/%s', $database));
  $options = array();
  foreach ($result AS $col) {
    $options[$col['_id']] = t('@name (@col)', array('@name' => $col['name'], '@col' => $col['collection']));
  }
  return $options;
}

function msm_pipeline_match_validate(&$widget, &$form_state, &$form) {
  $parents = $widget['#parents'];
  $value = &drupal_array_get_nested_value($form_state['values'], $parents);
  $value['match'] = $value['match']['value'];
}

function msm_pipeline_sort_validate(&$widget, &$form_state, &$form) {
  $parents = $widget['#parents'];
  $value = &drupal_array_get_nested_value($form_state['values'], $parents);
  $value['sort'] = $value['sort']['value'];
}

function msm_pipeline_stage_project_widget($stage, $stageajax) {
  $html_id = drupal_html_id(__FUNCTION__);
  $widget = array(
    '#prefix' => sprintf('<div id="%s">', $html_id),
    '#suffix' => '</div>',
  );
  $ajax = array(
    'callback' => 'msm_pipeline_stage_project_widget_ajax',
    'wrapper' => $html_id,
  );

  $widget['fields'] = array(
    '#theme' => 'msm_project_schema_table',
    '#tree' => TRUE,
  );

  $fields = msm_schema_flatten_fields($stage['schema'], MSM_SCHEMA_FLATTEN_ALL);

  $parent_options = msm_schema_field_options($stage['schema'], MSM_SCHEMA_FLATTEN_GROUPS);
  foreach ($fields AS $f => $fi) {
    $parts = explode('.', $f);
    $fi += array(
      'localName' => array_pop($parts),
      'parent' => implode('.', $parts),
      'weight' => 0,
    );
    $widget['fields'][$f] = array(
      '#tree' => TRUE,
    );
    $widget['fields'][$f]['localName'] = array(
      '#type' => 'textfield',
      '#title' => t('Local name'),
      '#title_display' => 'invisible',
      '#default_value' => $fi['localName'],
    );
    $widget['fields'][$f]['parent'] = array(
      '#type' => 'select',
      '#title' => t('Parent'),
      '#options' => array(t('-- root --')) + $parent_options,
      '#title_display' => 'invisible',
      '#default_value' => $fi['parent'],
    );

    $weights = range(-100, 100);
    $widget['fields'][$f]['weight'] = array(
      '#type' => 'select',
      '#title' => t('Weight'),
      '#options' => array_combine($weights, $weights),
      '#title_display' => 'invisible',
      '#default_value' => $fi['weight'],
    );
    $widget['fields'][$f]['actions'] = array(
    );
    $widget['fields'][$f]['actions']['remove'] = array(
      '#type' => 'checkbox',
      '#title' => t('Remove field'),
      '#ajax' => $ajax,
    );
  }

  $widget['add_field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add field'),
    '#attributes' => array('class' => array('container-inline')),
  );
  $widget['add_field']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
  );
  $widget['add_field']['parent'] = array(
    '#type' => 'select',
    '#title' => t('Parent'),
    '#options' => array(t('-- root --')) + msm_schema_field_options($stage['schema'], MSM_SCHEMA_FLATTEN_GROUPS),
  );
  $widget['add_field']['button'] = array(
    '#type' => 'button',
    '#value' => t('Add field'),
    '#ajax' => $stageajax,
  );
  #$widget['#element_validate'][] = 'msm_pipeline_stage_project_widget_validate';
  return $widget;
}

function msm_pipeline_stage_project_widget_ajax($form, &$form_state) {
  $ele = $form_state['triggering_element'];
  $parents = $ele['#parents'];
  array_pop($parents);
  array_pop($parents);
  array_pop($parents);
  dsm($form_state);
  return drupal_array_get_nested_value($form, $parents);
}

function msm_pipeline_stage_project_validate(&$element, &$form_state) {
  $parents = $element['#parents'];
  $value = &drupal_array_get_nested_value($form_state['values'], $parents);
  $flat_schema = msm_schema_flatten_fields($value['schema'], MSM_SCHEMA_FLATTEN_ALL);
  foreach ($value['project']['fields'] AS &$item) {
    unset($item['actions']);
  }
  $tele = $form_state['triggering_element'];
  if ($tele['#type'] == 'checkbox' && $tele['#title'] == t('Remove field')) {
    $parents = $tele['#parents'];
    array_pop($parents);
    array_pop($parents);
    $field = array_pop($parents);
    unset($element['project']['fields'][$field]);
    unset($value['project']['fields'][$field]);
    unset($flat_schema[$field]);
  }
  $merged = array_merge_recursive($value['project']['fields'], $flat_schema);
  $value['schema'] = msm_schema_expand_fields($merged);
  unset($value['project']);
}

function msm_pipeline_stage_group_widget($stage, $schema, $stageajax) {
  $widget = array(
  );
  $widget['add_field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add field'),
    '#attributes' => array('class' => array('container-inline')),
  );
  $widget['add_field']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
  );
  $widget['add_field']['parent'] = array(
    '#type' => 'select',
    '#title' => t('Parent'),
    '#options' => array(t('-- root --')) + msm_schema_field_options($schema, MSM_SCHEMA_FLATTEN_GROUPS),
  );
  $widget['add_field']['button'] = array(
    '#type' => 'button',
    '#value' => t('Add field'),
    '#ajax' => $stageajax,
  );
  return $widget;
}
