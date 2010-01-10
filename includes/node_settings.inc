<?php
// $Id: node_settings.inc,v 1.5 2009/09/21 05:23:02 dww Exp $


/**
 * @file
 * Code related to the per-node signup settings form.
 */

/**
 * Returns the form for the per-node signup settings.
 *
 * This is shared by the settings page and the node edit page.
 *
 * @param $node
 *   The fully loaded node object if we've got it.
 * @param $node_type
 *   The type of the node.  When creating new content, the caller can know the
 *   node type, even if $node is NULL.
 * @param $has_date
 *   Boolean flag indicating if this node (or site) has signup-aware
 *   date functionality, which is required for reminder emails to be in
 *   the form.
 * @param $include_buttons
 *   Boolean flag indicating if the form should include its own submit buttons.
 *
 * @return
 *   The form array for the per-node signup settings.
 *
 */
function signup_node_settings_form($form_state, $node = NULL, $node_type = NULL, $has_date = FALSE, $include_buttons = FALSE) {
  if (module_exists('token')) {
    $signup_token_description = t('Supported string substitutions: %node_title, %node_url, %node_start_time, %user_name, %user_mail, %user_signup_info (additional information from the signup form), %cancel_signup_url (access to this link is denied to users without the "%cancel_own_signups" permission), and any tokens in the %replacement_tokens list.', array('%replacement_tokens' => t('Replacement tokens'), '%cancel_own_signups' => t('cancel own signups')));
  }
  else {
    $signup_token_description = t('Supported string substitutions: %node_title, %node_url, %node_start_time, %user_name, %user_mail, %user_signup_info (additional information from the signup form), and %cancel_signup_url (access to this link is denied to users without the "%cancel_own_signups" permission).', array('%cancel_own_signups' => t('cancel own signups')));
  }

  // Load the default admin form data for new nodes.
  if (!$node || !$node->signup) {
    $result = db_fetch_object(db_query("SELECT * FROM {signup} WHERE nid = 0"));
    $node->signup_forwarding_email = $result->forwarding_email;
    $node->signup_send_confirmation = $result->send_confirmation;
    $node->signup_confirmation_email = $result->confirmation_email;
    $node->signup_send_reminder = $result->send_reminder;
    $node->signup_reminder_days_before = $result->reminder_days_before;
    $node->signup_reminder_email = $result->reminder_email;
    $node->signup_close_signup_limit = $result->close_signup_limit;
    $node->signup_form_panes = $result->form_panes;
  }
  
  // Get information about all potential form panes from modules that 
  // implement hook_signup_pane_info.
  $signup_form_pane_info = module_invoke_all('signup_pane_info');
  
  // Put in weights from either saved settings or for panes not enabled on this 
  // node a default heavy weight of 10.
  foreach ($signup_form_pane_info as $id => $signup_form_pane) {
    $signup_form_pane_info[$id]['weight'] = isset($node->signup_form_panes[$id]) ? $node->signup_form_panes[$id]['weight'] : 10;
  }
  // Sort the list by weight so it appears correctly in the UI.
  uasort($signup_form_pane_info, '_signup_cmp_weight'); 
  
  // The label and the theme function for the form panes table.
  $form['signup_form_panes'] = array(
    // This gets a label on the whole table; not sure this is the best approach though.
    // Improvements welcome.
    '#type' => 'item',
    '#title' => t('Signup form panes'),
    '#tree' => TRUE,
    '#description' => t('Enable the signup form panes you want to use on this content. You may also drag enabled panes into any order. The position of disabled panes in the list is ignored.'),
    '#theme' => 'signup_node_settings_form_panes',
  );
  // Add a warning if there are already existing signups.
  if ($node->signup_effective_total > 0) {
     $form['signup_form_panes']['#description'] .= t(' <span class="warning">There are existing signups recorded. If you disable panes their data will remain in your records; if you enable panes past signups will not have any data for them.</span>');
  }
  
  foreach ($signup_form_pane_info as $id => $signup_form_pane) {
    $form['signup_form_panes'][$id]['enabled'] = array(
      '#type' => 'checkbox',
      '#default_value' => isset($node->signup_form_panes[$id]),
      // Don't set the #title: we want it in a separate column. 
    );
    $form['signup_form_panes'][$id]['label'] = array(
      '#value' => $signup_form_pane['label'],
    );
    $form['signup_form_panes'][$id]['description'] = array(
      '#value' => $signup_form_pane['description'],
    );
    if (isset($signup_form_pane['operations'])) {
      $form['signup_form_panes'][$id]['operations'] = array(
        '#value' => $signup_form_pane['operations'],
      );
    }
    $form['signup_form_panes'][$id]['weight'] = array(
      '#type' => 'weight',
      '#delta' => 10,      
      '#default_value' => $signup_form_pane['weight'],
      '#attributes' => array(
        'class' => "signup-pane-weight",
      ),
    );
  }
  // Unset weight item if only 1 element
  if (count($signup_form_pane_info) == 1) {
    unset($form['signup_form_panes'][$id]['weight']);
  }

  $form['signup_forwarding_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Send signups to'),
    '#default_value' => $node->signup_forwarding_email,
    '#size' => 40, '#maxlength' => 64,
    '#description' => t('Email address where notification of new signups will be sent. Leave blank for no notifications.'),
  );
  $form['signup_send_confirmation'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send confirmation'),
    '#default_value' => $node->signup_send_confirmation,
  );
  $form['signup_confirmation_email'] = array(
    '#type' => 'textarea',
    '#title' => t('Confirmation email'),
    '#default_value' => $node->signup_confirmation_email,
    '#cols' => 40, '#rows' => 6,
    '#description' => t('Email sent to user upon signup. !token_description', array('!token_description' => $signup_token_description)),
  );
  if (module_exists('token')) {
    module_load_include('inc', 'signup', 'includes/token_help');
    _signup_token_help($form, 'signup_confirmation_token_fieldset');
  }

  if ($has_date) {
    // Define a sub-tree to wrap the next 2 form elements together in an
    // inline div for better display.
    $form['signup_reminder'] = array(
      '#prefix' => '<div class="container-inline">',
      '#suffix' => '</div>',
    );
    $form['signup_reminder']['signup_send_reminder'] = array(
      '#type' => 'checkbox',
      '#title' => t('Send reminder'),
      '#default_value' => $node->signup_send_reminder,
    );
    $options = array();
    for ($i = 1; $i <= 60; $i++) {
      $options[$i] = $i;
    }
    $node_type_name = isset($node_type) ? node_get_types('name', $node_type) : '';
    $form['signup_reminder']['signup_reminder_days_before'] = array(
      '#type' => 'select',
      '#default_value' => $node->signup_reminder_days_before,
      '#options' => $options,
      '#suffix' => !empty($node_type_name) ? t('day(s) before this %node_type', array('%node_type' => $node_type_name)) : t('day(s) before start time'),
    );
    $form['signup_reminder_email'] = array(
      '#type' => 'textarea',
      '#title' => t('Reminder email'),
      '#default_value' => $node->signup_reminder_email,
      '#cols' => 40, '#rows' => 6,
      '#description' =>  !empty($node_type_name) ? t('Email sent to user as a reminder before the %node_type starts. !token_description', array('%node_type' => $node_type_name, '!token_description' => $signup_token_description)) : t('Email sent to user as a reminder before the start time. !token_description', array('!token_description' => $signup_token_description)),
    );
    if (module_exists('token')) {
      module_load_include('inc', 'signup', 'includes/token_help');
      _signup_token_help($form, 'signup_reminder_token_fieldset');
    }
  }

  $form['signup_close_signup_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Signup limit'),
    '#default_value' => $node->signup_close_signup_limit,
    '#size' => 4, '#maxlength' => 8,
    '#description' => t('Maximum number of users who can sign up before signups are automatically closed. If set to 0, there is no limit.'),
    '#prefix' => '<div id="signup-limit">',
    '#suffix' => '</div>',
  );
  $form['signup'] = array('#type' => 'hidden', '#value' => 1);

  if ($include_buttons) {
    $form['#node'] = $node;
    $form['buttons']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
    $form['buttons']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset to defaults'),
    );
    $form['#submit'][] = 'signup_node_settings_form_submit';
  }

  return $form;
}

/**
 * Page callback for the node/N/signups/settings subtab.
 */
function signup_node_settings_page($node) {
  $node_scheduler = _signup_get_node_scheduler($node);
  $node_has_date = $node_scheduler != 'none';
  return drupal_get_form('signup_node_settings_form', $node, $node->type, $node_has_date, TRUE);
}

/**
 * Helper function to sort signup panes by their weight key.
 */
function _signup_cmp_weight($a, $b) {
  return $a['weight'] - $b['weight'];
}

/**
 * Theme function for the panes table in the node settings form.
 */
function theme_signup_node_settings_form_panes($form) {
  // Based on taxonomy.admin.inc
  foreach (element_children($form) as $key) {
    $row = array();
    $row[] = drupal_render($form[$key]['enabled']);
    if (isset($form[$key]['weight'])) {
      $form[$key]['weight']['#attributes']['class'] = 'signup-pane-weight';
      $row[] = drupal_render($form[$key]['weight']);
    }
    $row[] = drupal_render($form[$key]['label']);
    $row[] = drupal_render($form[$key]['description']);
    $row[] = drupal_render($form[$key]['operations']);
        
    $rows[] = array(
      'data' => $row,
      'class' => 'draggable',
    );
  }
  // A message if there are no rows.
  // @todo: figure out why the form label doesn't show in this case.
  if (!count($rows)) {
    $rows[] = array(
      array(
        'data' => t('No form panes are available.'),
        'colspan' => 3,
      )
    );
  }
  
  $header[] = t('Enabled');
  if (isset($form[$key]['weight'])) {
    $header[] = t('Weight');
  }
  $header[] = t('Name');
  $header[] = t('Description');
  $header[] = t('Operations');
  
  if (isset($form[$key]['weight'])) {  
    drupal_add_tabledrag('signup-node-settings-panes', 'order', 'sibling', 'signup-pane-weight');
  }
    
  $output = drupal_render($form); // ???
  $output .=  theme('table', $header, $rows, array('id' => 'signup-node-settings-panes'));
  return $output;
}

/**
 * Submit handler for the per-node signup settings form.
 *
 * @param $form_id
 *   The ID of the form being submitted.
 * @param $form_values
 *   The constructed form values array of the submitted form.
 */
function signup_node_settings_form_submit($form, &$form_state) {
  $node = $form['#node'];
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) {
    // If we're resetting, we just want to grab the site-wide defaults.
    $values = db_fetch_array(db_query("SELECT forwarding_email, send_confirmation, confirmation_email, close_signup_limit, send_reminder, reminder_days_before, reminder_email FROM {signup} WHERE nid = 0"));
    // Clear all the panes for the node.
    db_query("DELETE FROM {signup_panes} WHERE nid = %d", $node->nid);    
  }
  else {
    // Handle the pane settings.
    // WTF new nodes come here with ALL WEIGHTS set at 10???
    // @todo: new nodes probably need arbitrary weights setting?
    foreach ($form_state['values']['signup_form_panes'] as $pane_id => $form_state_values_panes) {
      if ($form_state_values_panes['enabled']) {
        $signup_panes[$pane_id] = $form_state_values_panes['weight'];
      }
    }
    // Clear all the panes for the node.
    db_query("DELETE FROM {signup_panes} WHERE nid = %d", $node->nid);    
    // If there are enabled panes, save them.
    if (count($signup_panes)) {
      // Give a singleton pane a weight value, arbitrarily set to 0.
      if (count($signup_panes) == 1) {
        // Convoluted way of getting the first (and only!) key to set the value to 0
        // @todo Suggestions for improvement welcome!
        reset($signup_panes);
        $pane_id = key($signup_panes);
        $signup_panes[$pane_id] = 0;
      }
      
      // Get the pane info so we know the callbacks. Callbacks are saved
      // (somewhat redundantly) into {signup_panes} so on node view and other
      // form displays we don't need to invoke the 'signup_pane_info':
      // some modules implementing this run queries, so it's potentially 
      // expensive.
      $signup_form_pane_info = module_invoke_all('signup_pane_info');
      
      // Sort by weight, preserve keys.
     // obsolete asort($signup_panes);
      foreach ($signup_panes as $pane_id => $weight) {
        db_query("INSERT INTO {signup_panes} (nid, pane_id, callback, weight) VALUES (%d, '%s', '%s', %d)", 
          $node->nid, $pane_id, $signup_form_pane_info[$pane_id]['callback'], $weight);
      }
    }
    
    // Populate $values from $form_state.
    foreach (array('forwarding_email', 'send_confirmation', 'confirmation_email', 'close_signup_limit') as $setting) {
      $values[$setting] = $form_state['values']["signup_$setting"];
    }
   
    // If we're dealing with a node that doesn't have a start time, these
    // fields are missing from the signup settings form, so we can't assume
    // they're defined.
    $values['send_reminder'] = isset($form_state['values']['signup_send_reminder']) ? $form_state['values']['signup_send_reminder'] : 0;
    $values['reminder_days_before'] = isset($form_state['values']['signup_reminder_days_before']) ? $form_state['values']['signup_reminder_days_before'] : 0;
    $values['reminder_email'] = isset($form_state['values']['signup_reminder_email']) ? $form_state['values']['signup_reminder_email'] : '';
  }

  // Either way, we want to make sure we're updating the values for the
  // current node, not nid 0...
  $values[] = $node->nid;
  db_query("UPDATE {signup} SET forwarding_email = '%s', send_confirmation = %d, confirmation_email = '%s', close_signup_limit = %d, send_reminder = %d, reminder_days_before = %d, reminder_email = '%s' WHERE nid = %d", $values);

  // See if the limit changed, and if so, take any necessary action.
  if ($node->signup_close_signup_limit != $form_state['values']['signup_close_signup_limit']) {
    $node->signup_close_signup_limit = $form_state['values']['signup_close_signup_limit'];
    $node->signup_effective_total = db_result(db_query("SELECT SUM(count_towards_limit) FROM {signup_log} WHERE nid = %d", $node->nid));
    _signup_check_limit($node, 'limit');
  }

  if ($op == t('Reset to defaults')) {
    drupal_set_message(t('The configuration options have been reset to their default values.'));
  }
  else {
    drupal_set_message(t('The configuration options have been saved.'));
  }
}

