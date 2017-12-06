<?php

function msm_schema_form($form, &$form_state, array $collection) {
  $module_path = drupal_get_path('module', 'msm');
  $form['#attached']['css'] = array(
    $module_path . '/css/schema-form.css',
  );
  $collection += array('type' => null);
  if (empty($form_state['values'])) {
    if (!empty($collection['schema'])) {
      $schema = $collection['schema'];
    }
    elseif (!empty($collection['source'])) {
      $source = msm_request(sprintf('collection/%s', $collection['source']));
      $schema = $source['schema'];
    }
    else $schema = array();
  }
  else {
    $schema = $form_state['schema'];
  }

  if (empty($collection['type']) && empty($collection['schema'])) {
    $form['analyze_schema'] = array(
      '#type' => 'link',
      '#title' => t('Analyze & Import Schema'),
      '#href' => sprintf('admin/structure/msm/db/%s/analyze-schema/%s', $collection['database'], $collection['collection']),
    );
  }

  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $ajax = array(
    'callback' => 'msm_schema_form_ajax',
    'wrapper' => $html_id,
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['schema'] = msm_schema_widget($schema, $ajax, $collection['type'] == 'custom');

  if ($collection['type'] == 'custom') {
    $form['add'] = array(
      '#tree' => TRUE,
      '#title' => t('Add new field'),
      '#type' => 'fieldset',
      '#attributes' => array('class' => array('container-inline')),
    );
    $form['add']['name'] = array(
      '#title' => t('Field name'),
      '#type' => 'textfield',
    );
    $form['add']['parent'] = array(
      '#title' => t('Parent'),
      '#type' => 'select',
      '#options' => msm_schema_parent_options($schema),
    );
    $form['add']['type'] = array(
      '#title' => t('Type'),
      '#type' => 'select',
      '#options' => msm_schema_field_type_options(),
    );
    $form['add']['button'] = array(
      '#type' => 'button',
      '#value' => t('Add field'),
      '#ajax' => $ajax,
    );
  }

  $form_state['schema'] = $schema;
  $form['#collection'] = $collection;
  return $form;
}

function msm_schema_form_validate(&$form, &$form_state) {
  $ele = $form_state['triggering_element'];
  $form_state['schema'] = $form_state['values']['schema'];
  if ($ele['#type'] == 'button' && $ele['#value'] == t('Add field')) {
    $add = $form_state['values']['add'];
    if (empty($add['name'])) {
      form_set_error('add][name', t('Field name is required.'));
    }
    else {
      $field = array('type' => $add['type']);
      if ($add['parent']) {
        $parents = explode('.', implode('.children.', explode('.', $add['parent'])));
        array_push($parents, 'children', $add['name']);
        if ($val = drupal_array_get_nested_value($form_state['schema'], $parents)) {
          dsm($val);
          form_set_error('add][name', t('Field exists'));
        }
        else {
          drupal_array_set_nested_value($form_state['schema'], $parents, $field, TRUE);
          unset($form_state['input']['add']);
        }
      }
      else {
        if (empty($form_state['schema'][$add['name']])) {
          $form_state['schema'][$add['name']] = $field;
          unset($form_state['input']['add']);
        }
        else form_set_error('add][name', t('Field exists'));
      }
    }
  }
  elseif ($ele['#type'] == 'checkbox' && $ele['#title'] == t('Remove field')) {
    $parents = $ele['#parents'];
    array_pop($parents);
    $field = array_pop($parents);
    $form_parents = $parents;
    array_shift($parents);
    $value = &drupal_array_get_nested_value($form_state['schema'], $parents);
    $widget = &drupal_array_get_nested_value($form, $form_parents);
    unset($value[$field]);
    unset($widget[$field]);
  }
}

function msm_schema_form_ajax($form, &$form_state) {
  return $form;
}

function msm_schema_form_submit($form, &$form_state) {
  $collectionId = $form['#collection']['_id'];
  $result = msm_post(sprintf('collection/%s/update', $collectionId), array(
    '$set' => array(
      'schema' => $form_state['schema'],
    ),
  ));
  if ($result['ok']) {
    if (!empty($form['#collection']['schemaImported'])) {
      $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/view', $collectionId);
    }
    else {
      $form_state['redirect'] = sprintf('admin/structure/msm/col/%s/settings', $collectionId);
    }
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_schema_widget($schema, array $ajax, $editable = FALSE, &$progress = array()) {
  $progress += array(
    'completed' => 0,
    'total' => 0,
  );
  $widget = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#tree' => TRUE,
    #'#attributes' => array('class' => array('container-inline', 'msm-schema-widget')),
  );
  $subprogress = array('total' => 0, 'completed' => 0);
  foreach ($schema AS $fn => $fi) {
    $fieldprogress = array('total' => 0, 'completed' => 0);
    $widget[$fn] = msm_schema_field_widget($fn, $fi, $fieldprogress, $ajax, $editable);
    $progress['completed'] = $subprogress['completed'] + $fieldprogress['completed'];
    $progress['total'] = $subprogress['total'] + $fieldprogress['total'];
  }
  $widget['#title'] = msm_schema_widget_progress_title(t('Schema'), $progress);
  $widget['#progress'] = $progress;
  return $widget;
}

function msm_schema_field_widget($name, $info, &$progress, array $ajax, $editable = FALSE) {
  $info += array(
    'operator' => null,
    'filterable' => null,
    'display' => array(
      'title' => null,
      'weight' => null,
      'abbreviation' => null,
      'priority' => null,
      'description' => null,
      'hidden' => null,
      'visible' => null,
    ),
  );
  msm_schema_widget_progress_increment($progress, $name, !empty($info['operator']));
  msm_schema_widget_progress_increment($progress, $name, !empty($info['display']['title']));
  msm_schema_widget_progress_increment($progress, $name, $info['display']['weight'] !== null);
  msm_schema_widget_progress_increment($progress, $name, !empty($info['display']['abbreviation']));
  msm_schema_widget_progress_increment($progress, $name, !empty($info['display']['priority']));
  msm_schema_widget_progress_increment($progress, $name, !empty($info['display']['description']));

  $html_id = drupal_html_id(__FUNCTION__);
  $fieldajax = array(
    'callback' => 'msm_schema_field_widget_ajax',
    'wrapper' => $html_id,
  );
  $widget = array(
    '#prefix' => sprintf('<div id="%s">', $html_id),
    '#suffix' => '</div>',
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => $name,
    '#tree' => TRUE,
    '#attributes' => array('class' => array('msm-schema-field-widget')),
  );
  if ($editable) {
    $widget['remove'] = array(
      '#title' => t('Remove field'),
      '#type' => 'checkbox',
      '#ajax' => $ajax,
    );
    $widget['type'] = array(
      '#title' => t('Field type'),
      '#type' => 'select',
      '#options' => msm_schema_field_type_options(empty($info['children']) || count($info['children'])),
      '#default_value' => $info['type'],
    );
  }
  else {
    $widget['type'] = array(
      '#type' => 'value',
      '#value' => $info['type'],
    );
  }
  $widget['operator'] = array(
    '#title' => t('Aggregation operator'),
    '#type' => 'select',
    '#options' => msm_schema_field_operator_options($info['type']),
    '#default_value' => $info['operator'] ? $info['operator'] : msm_schema_field_operator_default($info['type']),
  );
  $widget['filterable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Filterable'),
    '#default_value' => $info['filterable'],
    '#ajax' => $fieldajax,
  );
  if ($info['filterable']) {
    $info += array('filter' => array());
    $info['filter'] += array(
      'type' => null,
      'options' => null,
    );
    msm_schema_widget_progress_increment($progress, $name, !empty($info['filter']['type']));
    msm_schema_widget_progress_increment($progress, $name, !empty($info['filter']['options']));
    $widget['filter'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#title' => t('Filter options'),
    );
    $widget['filter']['type'] = array(
      '#type' => 'select',
      '#title' => t('Filter widget type'),
      '#options' => msm_schema_field_filter_type_options(),
      '#default_value' => $info['widget']['type'],
      '#description' => t('This should be utilized in front-end implementations.'),
    );
    $widget['filter']['options'] = array(
      '#type' => 'text_format',
      '#format' => 'catalyst_js',
      '#title' => t('Filter options'),
      '#default_value' => "return {\n\n};",
    );
  }
  $widget['display'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
    '#attributes' => array('class' => array('container-inline')),
  );
  $widget['display']['title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#default_value' => $info['display']['title'],
  );
  $widget['display']['abbreviation'] = array(
    '#title' => t('Abbreviation'),
    '#type' => 'textfield',
    '#size' => 12,
    '#default_value' => $info['display']['abbreviation'],
  );
  $weights = range(-25, 25);
  $widget['display']['weight'] = array(
    '#title' => t('Weight'),
    '#type' => 'select',
    '#options' => array_combine($weights, $weights),
    '#default_value' => $info['display']['weight'] === null ? 0 : $info['display']['weight'],
  );
  $widget['display']['priority'] = array(
    '#title' => t('Priority'),
    '#type' => 'select',
    '#options' => msm_schema_field_priority_options(),
    '#default_value' => $info['display']['priority'],
  );
  $widget['display']['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $info['display']['description'],
  );
  $widget['display']['hidden'] = array(
    '#title' => t('Hide from display'),
    '#type' => 'checkbox',
    '#default_value' => $info['display']['hidden'],
  );
  $widget['display']['visible'] = array(
    '#title' => t('Visible by default'),
    '#type' => 'checkbox',
    '#default_value' => $info['display']['visible'],
  );
  if (!empty($info['children'])) {
    $widget['children'] = msm_schema_widget($info['children'], $ajax, $editable, $progress);
  }
  if ($editable) {
    $widget['advanced'] = array(
      '#title' => t('Advanced'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $widget['advanced']['attributes'] = array(
      '#title' => t('Custom attributes (code)'),
      '#type' => 'text_format',
      '#format' => 'catalyst_js',
      '#default_value' => $info['attributes'],
    );
  }
  $widget['#title'] = msm_schema_widget_progress_title($name, $progress['fields'][$name]);
  return $widget;
}

function msm_schema_field_widget_ajax($form, &$form_state, &$context) {
  $ele = $form_state['triggering_element'];
  $parents = $ele['#parents'];
  array_pop($parents);
  $widget = drupal_array_get_nested_value($form, $parents);
  $widget['#collapsed'] = FALSE;
  $widget['#attributes']['class'] = array('collapsible');
  return $widget;
}

function msm_schema_field_priority_options() {
  return array();
}

function msm_schema_parent_options($schema) {
  $fields = msm_schema_flatten_fields($schema, MSM_SCHEMA_FLATTEN_GROUPS);
  $options = array(t('-- root --'));
  foreach ($fields AS $name => $field) {
    if (in_array($field['type'], array('group', 'array'))) {
      $options[$name] = empty($field['display']['title']) ? $name : sprintf('%s (%s)', $field['display']['title'], $name);
    }
  }
  return $options;
}

function msm_schema_field_type_options($has_children = FALSE) {
  $options = array(
    'group' => t('Group'),
    'array' => t('Array'),
  );
  if (!$has_children) {
    $options += array(
      'text' => t('Text'),
      'int' => t('Number'),
      'float' => t('Decimal'),
    );
  }
  return $options;
}

function msm_schema_widget_progress_title($field_name, $progress) {
  return t('@field_name - @completed/@total - @completed_percentage% completed', array(
    '@field_name' => $field_name,
    '@completed_percentage' => $progress['completed']
      ? number_format(($progress['completed'] / $progress['total']) * 100)
      : 0,
    '@completed' => $progress['completed'],
    '@total' => $progress['total'],
  ));
}

function msm_schema_widget_progress_increment(&$progress, $field_name = null, $completed = null) {
  if ($field_name) {
    if (empty($progress['fields'][$field_name])) {
      $progress['fields'][$field_name] = array('completed' => 0, 'total' => 0);
    }
    $progress['fields'][$field_name]['total']++;
    if ($completed) $progress['fields'][$field_name]['completed']++;
  }
  $progress['total']++;
  if ($completed) $progress['completed']++;
}

function msm_schema_field_operator_options($field_type) {
  $options = array(
    'push' => t('$push'),
    'last' => t('$last'),
    'first' => t('$first'),
    'sum' => t('$sum'),
    'avg' => t('$avg'),
  );
  switch ($field_type) {
    default:
  }
  return $options;
}

function msm_schema_field_operator_default($field_type) {
  switch ($field_type) {
    case 'group': return 'push';
    case 'array': return 'push';
    case 'text': return 'last';
    case 'int': return 'sum';
  }
}

function msm_schema_field_filter_type_options() {
  return array(
    'selectOne' => t('Select'),
    'selectMany' => t('Select Multiple'),
  );
}
