<?php
// $Id: signup_handler_field_signup_user_form_data.inc,v 1.2 2009/01/07 00:55:00 dww Exp $

/**
 * Field handler to display custom (site-specific) signup form data.
 */
class signup_handler_field_signup_user_form_data extends views_handler_field_prerender_list {
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['form_data_fieldname'] = array(
      '#type' => 'textfield',
      '#title' => t(' Field Name'),
      '#description' => t('If you want to display a single field from the signup form (see theme/signup_form.inc), enter its name here. Otherwise, all fields will be displayed.'),
      '#default_value' => isset($this->options['form_data_fieldname']) ? $this->options['form_data_fieldname'] : '',
    );
  }
  
  /**
   * Ensure we have the node nid and signup sid values to pass to 
   * hook_signup_form_data_display_alter().
   */
  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();

    parent::query();
  }

  /**
   * Set each field value to "$key: $value" when rendering all fields.
   *
   * @see theme_signup_custom_data_field_text()
   */
  function pre_render($values) {
    if (!$this->options['form_data_fieldname']) {
      foreach ($values as $key => $result) {
        $form_data = unserialize($result->{$this->field_alias});
        if ($form_data) {
          $sid = $result->{$this->aliases['sid']};
          $nid = $result->{$this->aliases['nid']};
          // Let pane modules alter the data for output.
          // This may involve internal data being removed.
          drupal_alter('signup_form_data_display', $form_data, $nid, $sid);
          foreach ($form_data as $pane_id => $pane_data) {
            foreach ((array)$pane_data as $key => $value) {
              $this->items[$result->{$this->field_alias}][$key] = check_plain(theme('signup_custom_data_field_text', $key, $value));
            }
          }
        }
      }
    }
  }

  function render($values) {
    if ($this->options['form_data_fieldname']) {
      $form_data = unserialize($values->{$this->field_alias});
      return check_plain($form_data[$this->options['form_data_fieldname']]);
    }
    else {
      return parent::render($values);
    }
  }
}

